// This file is part of DynExp.

#include "stdafx.h"
#include "NP_Conex_CC.h"
#include <typeinfo>

namespace DynExpInstr
{
	// Initialize:
	// This function is called, when the Instrument is started in DynExp.
	void NP_Conex_CC_Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter()); // dynamic cast of parameters returned by Instance.ParamsGetter() to the type NP_Conex_CC
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter()); // Pointer to the InstrParams and InstrData

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter); // The hardware adapters of InstrParams and InstrData are locked here.
			InstrData->HardwareAdapter->Clear(); // clears the hardware adapter

			// Define and go to home (this includes resetting the controller)
			// To set the current position as home position, the stage has to be in the CONFIGURATION state. This state can only be reached from the NOT REFERENCED state. 
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "RS"; // Go to NOT REFERENCED state.
			std::this_thread::sleep_for(std::chrono::milliseconds(500)); // This takes 500 ms.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW1"; // Go to CONFIGURATION state.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "HT1"; // Set current position to home position.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW0"; // Go to NOT REFERENCED state.
			std::this_thread::sleep_for(std::chrono::seconds(3)); // This takes 3 s.
			*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "OR"; // Got to READY state.

		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance); // In case the init function is extended
	}

	// Close the controller:
	void NP_Conex_CC_Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter()); // dynamic cast of parameters returned by Instance.ParamsGetter() to the type NP_Conex_CC
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
			*InstrData->HardwareAdapter << AbortMotion;
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->operator<<() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	// Update status of the stage:
	// Asks for position, velocity and status and updates the corresponding variables (that might be displayed in the GUI):
	void NP_Conex_CC_Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				// Tell status
				auto TellStatus = Util::ToStr(InstrParams->ConexAddress.Get()) + "TS";
				*InstrData->HardwareAdapter << TellStatus; // The output will be something like '1TS000033'.
				auto StatusAnswer = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "TS");
				std::stringstream StatusStream(StatusAnswer);
				StatusStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);

				uint16_t ErrorMap; // Variable to hold the error map (16 bits)
				char ErrorMapBuffer[5]{ 0 };  // 4 characters + 1 for null terminator
				StatusStream.read(ErrorMapBuffer, 4);  // Read the first 4 characters
				std::istringstream ErrorMapStream(ErrorMapBuffer);
				int tempErrorMap;
				ErrorMapStream >> std::hex >> tempErrorMap;
				ErrorMap = static_cast<uint16_t>(tempErrorMap);

				if (ErrorMap > 0xFFFF) // Validate that it fits in 16 bits
					throw Util::InvalidDataException("Received an unexpected Conex-CC error map.");
				InstrData->ErrorCode = static_cast<NP_Conex_CCStageData::ErrorCodeType>(ErrorMap); // Only 0 is no error

				uint8_t State;     // Variable to hold the state (8 bits)
				char StateBuffer[3]{ 0 };  // 2 characters + 1 for null terminator
				StatusStream.read(StateBuffer, 2);  // Read the next 2 characters
				std::istringstream StateStream(StateBuffer);
				int tempState;
				StateStream >> std::hex >> tempState;
				State = static_cast<uint8_t>(tempState);

				if (State > 0xFF) // Validate that it fits in 8 bits
					throw Util::InvalidDataException("Received an unexpected Conex-CC status.");
				InstrData->Conex_CCStatus.Set(State);

				// Ask if stage is in Ready state
				// This is useful for two reaons. 
				// 1. The SetHomeTask and the ReferenceTask, require the stage to be set into CONFIGURATION state. To return from this state to READY mode takes 3 s.
				// 2. Sometimes the state jumps into a different state. If this is the case, it should go into READY state again. 
				auto Conex_CCStatus = InstrData->GetConex_CCStatus();
				if (Conex_CCStatus.NotReferencedFromReset() || Conex_CCStatus.NotReferencedFromHoming() || Conex_CCStatus.NotReferencedFromConfiguration()
					|| Conex_CCStatus.NotReferencedFromDisable() || Conex_CCStatus.NotReferencedFromReady() || Conex_CCStatus.NotReferencedFromMoving()
					|| Conex_CCStatus.NotReferencedNoParams() || Conex_CCStatus.Configuration() || Conex_CCStatus.DisableFromReady()
					|| Conex_CCStatus.DisableFromMoving() || Conex_CCStatus.DisableFromTracking() || Conex_CCStatus.DisableFromReadyT()
					|| Conex_CCStatus.TrackingFromReadyT() || Conex_CCStatus.TrackingFromTracking())
				{
					*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "OR";
				}

				// Tell position
				auto TellPosition = Util::ToStr(InstrParams->ConexAddress.Get()) + "TP";
				*InstrData->HardwareAdapter << TellPosition; // The output will be something like '1TP10.0000164'.
				auto PositionalAnswer = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "TP"); // The time to wait has to be at least 250 ms. Otherwise the answer has not been send yet. But then the stage always loses connection. If I set it to 25, it works, since eventually, it will read out the correct position from any previous answer. 
				InstrData->SetCurrentPosition(Util::StrToT<PositionerStageData::PositionType>(PositionalAnswer));

				// Tell programmed velocity
				auto TellVelocity = Util::ToStr(InstrParams->ConexAddress.Get()) + "VA?";
				*InstrData->HardwareAdapter << TellVelocity; // The output will be something like '1VA10'.
				auto VelocityAnswer = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "VA");
				InstrData->SetVelocity(Util::StrToT<PositionerStageData::PositionType>(VelocityAnswer));
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

		// Set the home search type (see InitTask), homing actually happens in the UpdateTask:
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "RS";
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW1";
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "HT1"; // use current position as HOME
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW0";

		return {};
	}

	// Find the absolute zero position:
	DynExp::TaskResultType NP_Conex_CC_Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		// It is not possible to use an if-condition, since the update of the Conex_CCStatus is so slow.
		auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		*InstrData->HardwareAdapter << AbortMotion;
		//std::this_thread::sleep_for(std::chrono::milliseconds(300)); // It takes 300 ms until the next command can be recognized. So, if this line is not used, this new task only aborts the old task but is not actually executed.

		// Save current velocity (should be done before every reset command)
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner()); // for GetDefaultVelocity
		auto DefaultVelocity = Owner->GetDefaultVelocity();
		auto TellVelocity = Util::ToStr(InstrParams->ConexAddress.Get()) + "VA?";
		*InstrData->HardwareAdapter << TellVelocity;
		// auto CurrentVelocity = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(250)), "VA"); // Q: Why does this function give an error?

		// Define home (see InitTask):
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "RS";
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW1";
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "HT2"; // use MZ switch only
		*InstrData->HardwareAdapter << Util::ToStr(InstrParams->ConexAddress.Get()) + "PW0";

		return {};
	}

	// 6. Set the velocity:
	DynExp::TaskResultType NP_Conex_CC_Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Set velocity
		auto SetVelocity = Util::ToStr(InstrParams->ConexAddress.Get()) + "VA" + Util::ToStr(Velocity);
		*InstrData->HardwareAdapter << SetVelocity;

		return {};
	}

	// 7. Go to home position:
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		// It is not possible to use an if-condition, since the update of the Conex_CCStatus is so slow.
		auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		*InstrData->HardwareAdapter << AbortMotion;
		//std::this_thread::sleep_for(std::chrono::milliseconds(300)); // It takes 300 ms until the next command can be recognized. So, if this line is not used, this new task only aborts the old task but is not actually executed.

		// Go home
		auto GoHome = Util::ToStr(InstrParams->ConexAddress.Get()) + "PA0";
		*InstrData->HardwareAdapter << GoHome;

		return {};
	}

	// 8. Move to an absolute position: 
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		// It is not possible to use an if-condition, since the update of the Conex_CCStatus is so slow.
		auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		*InstrData->HardwareAdapter << AbortMotion;
		//std::this_thread::sleep_for(std::chrono::milliseconds(300)); // It takes 300 ms until the next command can be recognized. So, if this line is not used, this new task only aborts the old task but is not actually executed.

		// Move absolute
		auto MoveAbsolute = Util::ToStr(InstrParams->ConexAddress.Get()) + "PA" + Util::ToStr(Position);
		*InstrData->HardwareAdapter << MoveAbsolute;

		return {};
	}

	// 9. Move by a distance (to a relative position):
	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		// It is not possible to use an if-condition, since the update of the Conex_CCStatus is so slow.
		auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		*InstrData->HardwareAdapter << AbortMotion;
		//std::this_thread::sleep_for(std::chrono::milliseconds(300)); // It takes 300 ms until the next command can be recognized. So, if this line is not used, this new task only aborts the old task but is not actually executed.

		// Move relative
		auto MoveRelative = Util::ToStr(InstrParams->ConexAddress.Get()) + "PR" + Util::ToStr(Position); // Q: Why is position an integer? With this I cannot make small steps.
		*InstrData->HardwareAdapter << MoveRelative;

		return {};
	}

	// 10. Abort motion:
	DynExp::TaskResultType NP_Conex_CC_Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// Abort motion
		// It is not possible to use an if-condition, since the update of the Conex_CCStatus is so slow.
		auto AbortMotion = Util::ToStr(InstrParams->ConexAddress.Get()) + "ST";
		*InstrData->HardwareAdapter << AbortMotion;
		//std::this_thread::sleep_for(std::chrono::milliseconds(300)); // It takes 300 ms until the next command can be recognized. So, if this line is not used, this new task only aborts the old task but is not actually executed.

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
		auto StageIsMoving = Conex_CCStatus.Moving() || Conex_CCStatus.Homing();
		return StageIsMoving;
	}

	bool NP_Conex_CCStageData::HasArrivedChild() const noexcept
	{
		auto StageHasArrived = Conex_CCStatus.ReadyFromHoming() || Conex_CCStatus.ReadyFromMoving() || Conex_CCStatus.ReadyFromDisable()
			|| Conex_CCStatus.ReadyTFromReady() || Conex_CCStatus.ReadyTFromTracking() || Conex_CCStatus.ReadyTFromDisableT();
		return StageHasArrived;
	}

	bool NP_Conex_CCStageData::HasFailedChild() const noexcept
	{
		return ErrorCode != 0
			|| Conex_CCStatus.NotReferencedFromReset() || Conex_CCStatus.NotReferencedFromHoming() || Conex_CCStatus.NotReferencedFromConfiguration()
			|| Conex_CCStatus.NotReferencedFromDisable() || Conex_CCStatus.NotReferencedFromReady() || Conex_CCStatus.NotReferencedFromMoving()
			|| Conex_CCStatus.NotReferencedNoParams() || Conex_CCStatus.Configuration() || Conex_CCStatus.DisableFromReady()
			|| Conex_CCStatus.DisableFromMoving() || Conex_CCStatus.DisableFromTracking() || Conex_CCStatus.DisableFromReadyT()
			|| Conex_CCStatus.TrackingFromReadyT() || Conex_CCStatus.TrackingFromTracking();
	}

	// StartCode is nnAA, nn is the controller adress, AA is the command name, e.g. "TP" for tell position
	std::string NP_Conex_CC::AnswerToNumberString(std::string&& Answer, const char* StartCode)
	{
		auto Pos = Answer.find(StartCode);

		if (Answer.empty())
			throw Util::InvalidDataException("Received an empty answer.");
		else if (Pos == std::string::npos)
			throw Util::InvalidDataException("Received an unexpected answer.");

		auto TmpReturnValue = Answer.substr(Pos + 2);
		auto TmpReturnValueType = typeid(TmpReturnValue).name();

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
		auto AbortMotion = "ST"; // Without controller address, the movement is stopped on all controllers.
		*InstrData->HardwareAdapter << AbortMotion;
	}

	void NP_Conex_CC::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<NP_Conex_CC>());
	}
}