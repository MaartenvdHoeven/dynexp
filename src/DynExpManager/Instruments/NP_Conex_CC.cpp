// This file is part of DynExp.

#include "stdafx.h"
#include "NP_Conex_CC.h"

namespace DynExpInstr
{
	// 1. Initialize (reset and select controller):
	// Q: When exactly is this function called?
	void NP_Conex_CC_Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
			InstrData->HardwareAdapter->Clear();

			// Reset and select controller.
			*InstrData->HardwareAdapter << "1RS"; 
			*InstrData->HardwareAdapter << std::string(1, static_cast<char>(1)) + InstrParams->ConexAddress.Get();

			// Ignore version and factory string and await startup.
			InstrData->HardwareAdapter->WaitForLine(3, std::chrono::milliseconds(50)); // Q: What waiting time is this? Does it need to be adjusted?
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	// 2. Stop the task (abort motion):
	void NP_Conex_CC_Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			*InstrData->HardwareAdapter << "1ST";
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->operator<<() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	// 3. Asks for position, velocity and status and updates the corresponding variables (that might be displayed in the GUI):
	// Q: What is this function doing?
	void NP_Conex_CC_Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				// Tell position
				*InstrData->HardwareAdapter << "TP"; // TODO: not the right command yet
				InstrData->SetCurrentPosition(Util::StrToT<PositionerStageData::PositionType>(
					NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "P:")));

				// Tell programmed velocity
				*InstrData->HardwareAdapter << "TY"; // TODO: not the right command yet
				InstrData->SetVelocity(Util::StrToT<PositionerStageData::PositionType>(
					NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "Y:")));

				// Tell status
				*InstrData->HardwareAdapter << "TS"; // TODO: not the right command yet
				std::stringstream StatusStream(NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "S:"));
				StatusStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
				StatusStream << std::hex;

				uint16_t Byte;			// Since stringstream handles uint8_t as character...
				StatusStream >> Byte;	// Extract Conex_CC status
				if (Byte > 0xFF)
					throw Util::InvalidDataException("Received an unexpected Conex_CC status.");
				InstrData->Conex_CCStatus.Set(static_cast<uint8_t>(Byte));

				StatusStream.seekg(15);
				StatusStream >> Byte;	// Extract error codes
				if (Byte > 0xFF)
					throw Util::InvalidDataException("Received an unexpected error code.");
				InstrData->ErrorCode = static_cast<NP_Conex_CCStageData::ErrorCodeType>(Byte);
			}
			catch ([[maybe_unused]] const Util::InvalidDataException& e)
			{
				UpdateError = true;

				// Swallow if just one or two subsequent updates failed.
				if (InstrData->NumFailedStatusUpdateAttempts++ >= 3)
					throw;
			}

			if (!UpdateError)
				InstrData->NumFailedStatusUpdateAttempts = 0;
		}
		// Issued if a mutex is blocked by another operation.
		catch (const Util::TimeoutException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}
		// Issued by NP_Conex_CC::AnswerToNumberString() or StrToT() if unexpected or no data has been received.
		catch (const Util::InvalidDataException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}
		// Issued by std::stringstream if extracting data fails.
		catch (const std::stringstream::failure& e)
		{
			Instance.GetOwner().SetWarning(e.what(), Util::DynExpErrorCodes::InvalidData);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	// 4. Set (define) home position:
	DynExp::TaskResultType NP_Conex_CC_Tasks::SetHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Define home
		*InstrData->HardwareAdapter << "1OR"; // I think this needs more commands: change to configuration state and back etc. 

		return {};
	}

	// 5. Find the front or back edge:
	DynExp::TaskResultType NP_Conex_CC_Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Find edge
		if (Direction == PositionerStage::DirectionType::Forward)
			*InstrData->HardwareAdapter << "1PA0";
		else
			*InstrData->HardwareAdapter << "1PA0";

		return {};
	}

	// 6. Set the velocity:
	DynExp::TaskResultType NP_Conex_CC_Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Set velocity
		*InstrData->HardwareAdapter << "1VA" + Util::ToStr(Velocity);

		return {};
	}

	// 7. Go to home position:
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter()); // Q: What is this InstrData variable?

		// Go home
		*InstrData->HardwareAdapter << "1PA0"; // Q: Sends a string via the hardware adapter?

		return {};
	}

	// 8. Move to an absolute position: 
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Move absolute
		*InstrData->HardwareAdapter << "1PA" + Util::ToStr(Position);

		return {};
	}

	// 9. Move by a distance (to a relative position):
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Move relative
		*InstrData->HardwareAdapter << "1PR" + Util::ToStr(Position);

		return {};
	}

	// 10. Abort motion:
	DynExp::TaskResultType NP_Conex_CC_Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		*InstrData->HardwareAdapter << "1ST";

		return DynExp::TaskResultType();
	}

	// Q: What are the functions below?
	void NP_Conex_CCStageData::ResetImpl(dispatch_tag<PositionerStageData>)
	{
		Conex_CCStatus.Set(0);
		ErrorCode = NoError;
		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<NP_Conex_CCStageData>());
	}

	bool NP_Conex_CCStageData::IsMovingChild() const noexcept
	{
		return !Conex_CCStatus.TrajectoryComplete();
	}

	bool NP_Conex_CCStageData::HasArrivedChild() const noexcept
	{
		return Conex_CCStatus.TrajectoryComplete();
	}

	bool NP_Conex_CCStageData::HasFailedChild() const noexcept
	{
		return ErrorCode != 0
			|| Conex_CCStatus.CommandError() || Conex_CCStatus.PositionLimitExceeded()
			|| Conex_CCStatus.ExcessivePositionError() || Conex_CCStatus.BreakpointReached();
	}

	// StartCode is something like "X:". In any case, it has a length of two characters
	std::string NP_Conex_CC::AnswerToNumberString(std::string&& Answer, const char* StartCode)
	{
		auto Pos = Answer.find(StartCode);

		if (Pos == std::string::npos)
			throw Util::InvalidDataException("Received an unexpected answer.");

		return Answer.substr(Pos + 2);
	}

	NP_Conex_CC::NP_Conex_CC(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	void NP_Conex_CC::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(GetInstrumentData());

		// Abort motion.
		*InstrData->HardwareAdapter << "1ST";
	}

	void NP_Conex_CC::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<NP_Conex_CC>());
	}
}