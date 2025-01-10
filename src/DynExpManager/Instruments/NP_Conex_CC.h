// This file is part of DynExp.

/**
 * @file NP_Conex_CC.h
 * @brief Implementation of an instrument to control the Newport rotation stage with the NP_Conex_CC controller.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Stage.h"

namespace DynExpInstr
{
	class NP_Conex_CC; // forward declaration

	namespace NP_Conex_CC_Tasks
	{
		class InitTask : public PositionerStageTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public PositionerStageTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public PositionerStageTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class SetHomeTask final : public DynExp::TaskBase
		{
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ReferenceTask final : public DynExp::TaskBase
		{
		public:
			ReferenceTask(PositionerStage::DirectionType Direction, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Direction(Direction) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStage::DirectionType Direction;
		};

		class SetVelocityTask final : public DynExp::TaskBase
		{
		public:
			SetVelocityTask(PositionerStageData::PositionType Velocity) noexcept : Velocity(Velocity) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Velocity;
		};

		class MoveToHomeTask final : public DynExp::TaskBase
		{
		public:
			MoveToHomeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class MoveAbsoluteTask final : public DynExp::TaskBase
		{
		public:
			MoveAbsoluteTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Position(Position) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Position;
		};

		class MoveRelativeTask final : public DynExp::TaskBase
		{
		public:
			MoveRelativeTask(PositionerStageData::PositionType Position, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Position(Position) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const PositionerStageData::PositionType Position;
		};

		class StopMotionTask final : public DynExp::TaskBase
		{
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class NP_Conex_CCStageData : public PositionerStageData
	{
		friend class NP_Conex_CC_Tasks::UpdateTask;

	public:
		struct Conex_CCStatusType
		{
			constexpr void Set(uint8_t ByteCode) noexcept { this->ByteCode = ByteCode; }

			constexpr bool Busy() const noexcept { return ByteCode & (1 << 0); }
			constexpr bool CommandError() const noexcept { return ByteCode & (1 << 1); }
			constexpr bool TrajectoryComplete() const noexcept { return ByteCode & (1 << 2); }
			constexpr bool IndexPulseReceived() const noexcept { return ByteCode & (1 << 3); }
			constexpr bool PositionLimitExceeded() const noexcept { return ByteCode & (1 << 4); }
			constexpr bool ExcessivePositionError() const noexcept { return ByteCode & (1 << 5); }
			constexpr bool BreakpointReached() const noexcept { return ByteCode & (1 << 6); }
			constexpr bool MotorLoopOff() const noexcept { return ByteCode & (1 << 7); }

		private:
			uint8_t ByteCode = 0;
		};

		enum ErrorCodeType : uint8_t {
			NoError,
			CommandNotFound,
			FirstCommandCharacterWasNotALetter,
			CharacterFollowingCommandWasNotADigit = 5,
			ValueTooLarge,
			ValueTooSmall,
			ContinuationCharacterWasNotAComma,
			CommandBufferOverflow,
			MacroStorageOverflow
		};

		NP_Conex_CCStageData() = default;
		virtual ~NP_Conex_CCStageData() = default;

		auto GetConex_CCStatus() const noexcept { return Conex_CCStatus; }
		auto GetErrorCode() const noexcept { return ErrorCode; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<PositionerStageData>) override final;
		virtual void ResetImpl(dispatch_tag<NP_Conex_CCStageData>) {};

		virtual bool IsMovingChild() const noexcept override;
		virtual bool HasArrivedChild() const noexcept override;
		virtual bool HasFailedChild() const noexcept override;

		Conex_CCStatusType Conex_CCStatus;
		ErrorCodeType ErrorCode = NoError;
		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class NP_Conex_CC_Params : public PositionerStageParams
	{
	public:
		NP_Conex_CC_Params(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : PositionerStageParams(ID, Core) {}
		virtual ~NP_Conex_CC_Params() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NP_Conex_CC_Params"; }

		Param<DynExp::ObjectLink<DynExp::SerialCommunicationHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "Serial port", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> ConexAddress = { *this, "ConexAddress", "Conex address",
			"Address (0-F) of the Conex controller to be used", true, "0" };

	private:
		void ConfigureParamsImpl(dispatch_tag<PositionerStageParams>) override final { ConfigureParamsImpl(dispatch_tag<NP_Conex_CC_Params>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NP_Conex_CC_Params>) {}
	};

	class NP_Conex_CC_Configurator : public PositionerStageConfigurator
	{
	public:
		using ObjectType = NP_Conex_CC;
		using ParamsType = NP_Conex_CC_Params;

		NP_Conex_CC_Configurator() = default;
		virtual ~NP_Conex_CC_Configurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NP_Conex_CC_Configurator>(ID, Core); }
	};

	class NP_Conex_CC : public PositionerStage
	{
	public:
		using ParamsType = NP_Conex_CC_Params;
		using ConfigType = NP_Conex_CC_Configurator;
		using InstrumentDataType = NP_Conex_CCStageData;

		static std::string AnswerToNumberString(std::string&& Answer, const char* StartCode);

		constexpr static auto Name() noexcept { return "NP Conex-CC"; } // A static function defines the name that appears in the dropdown menu of the GUI.

		NP_Conex_CC(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NP_Conex_CC() {}

		virtual std::string GetName() const override { return Name(); }

		virtual PositionerStageData::PositionType GetMinPosition() const noexcept override { return -12455000; }
		virtual PositionerStageData::PositionType GetMaxPosition() const noexcept override { return 12455000; }
		virtual PositionerStageData::PositionType GetResolution() const noexcept override { return 1000; }
		virtual PositionerStageData::PositionType GetMinVelocity() const noexcept override { return 0; }
		virtual PositionerStageData::PositionType GetMaxVelocity() const noexcept override { return 500000; }
		virtual PositionerStageData::PositionType GetDefaultVelocity() const noexcept override { return 200000; }

		virtual void SetHome() const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::SetHomeTask>(); }
		virtual void Reference(DirectionType Direction = DirectionType::Forward, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::ReferenceTask>(Direction, CallbackFunc); }
		virtual void SetVelocity(PositionerStageData::PositionType Velocity) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::SetVelocityTask>(Velocity); }

		virtual void MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveToHomeTask>(CallbackFunc); }
		virtual void MoveAbsolute(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveAbsoluteTask>(Position, CallbackFunc); }
		virtual void MoveRelative(PositionerStageData::PositionType Position, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::MoveRelativeTask>(Position, CallbackFunc); }
		virtual void StopMotion() const override { MakeAndEnqueueTask<NP_Conex_CC_Tasks::StopMotionTask>(); }

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<PositionerStage>) override final;
		virtual void ResetImpl(dispatch_tag<NP_Conex_CC>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NP_Conex_CC_Tasks::UpdateTask>(); }
	};
}