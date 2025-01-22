// This file is part of DynExp.

#include "stdafx.h"
#include "NP_Conex_CC.h"
#include <cstring>

namespace DynExpInstr
{
	// 1. Initialize (reset and select controller):
	// This function is called, when the Instrument is started in DynExp.
	void NP_Conex_CC_Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter()); // dynamic cast of parameters returned by Instance.ParamsGetter() to the type NP_Conex_CC
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter()); // Pointer to the InstrParams and InstrData

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter); // The hardware adapters of InstrParams and InstrData are locked here.
			InstrData->HardwareAdapter->Clear(); // clears the hardware adapter

			// Reset and select controller.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "RS";
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "OR";

			// Ignore version and factory string and await startup.
			InstrData->HardwareAdapter->WaitForLine(3, std::chrono::milliseconds(50)); // Try out these parameters. Try 3 times and wait for 50 ms each time.
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	// 2. Close the controller:
	void NP_Conex_CC_Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->operator<<() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	// 3. Asks for position, velocity and status and updates the corresponding variables (that might be displayed in the GUI):
	void NP_Conex_CC_Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				// Tell position
				*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "TP"; // The output will be something like '1TP10.0000164'. The next line transforms this into the expected format.
				InstrData->SetCurrentPosition(Util::StrToT<PositionerStageData::PositionType>(
					NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "TP")));

				// Tell programmed velocity
				*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "VA?"; // The output will be something like '1VA10'. The next line transforms this into the expected format.
				InstrData->SetVelocity(Util::StrToT<PositionerStageData::PositionType>(
					NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "VA")));

				// Tell status
				*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "TS"; // The output will be something like '1TS000033'. The next line transforms this into the expected format.
				std::stringstream StatusStream(NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "TS"));
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
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Define home
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "OR"; // I think this needs more commands: change to configuration state and back etc. 

		return {};
	}

	// 5. Find the front or back edge:
	DynExp::TaskResultType NP_Conex_CC_Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Find edge
		if (Direction == PositionerStage::DirectionType::Forward)
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PA0";
		else
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PA0";

		return {};
	}

	// 6. Set the velocity:
	DynExp::TaskResultType NP_Conex_CC_Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Set velocity
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "VA" + Util::ToStr(Velocity);

		return {};
	}

	// 7. Go to home position:
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Go home
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PA0";

		return {};
	}

	// 8. Move to an absolute position: 
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Move absolute
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PA" + Util::ToStr(Position);

		return {};
	}

	// 9. Move by a distance (to a relative position):
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Move relative
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PR" + Util::ToStr(Position);

		return {};
	}

	// 10. Abort motion:
	DynExp::TaskResultType NP_Conex_CC_Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";

		return DynExp::TaskResultType();
	}

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

	// StartCode is nnAA, nn is the controller adress, AA is the command name, e.g. "TP" for tell position
	std::string NP_Conex_CC::AnswerToNumberString(std::string&& Answer, const char* StartCode)
	{
		auto Pos = Answer.find(StartCode); // Q: Why is Answer an empty string?

		if (Pos == std::string::npos) 
			throw Util::InvalidDataException("Received an unexpected answer.");

		return Answer.substr(Pos + std::strlen(StartCode)); // transform, e.g. '1TP10.0000164' into '10.0000164'
	}

	NP_Conex_CC::NP_Conex_CC(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	void NP_Conex_CC::OnErrorChild() const
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(GetInstrumentData());

		// Abort motion.
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
	}

	void NP_Conex_CC::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<NP_Conex_CC>());
	}
}