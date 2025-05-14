// This file is part of DynExp.

/**
 * @file QutoolsQuTAGStandard.h
 * @brief Implementation of an instrument to control a single input of the qutools TDC
 * time tagger.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterQutoolsStandardTDC.h"
#include "MetaInstruments/TimeTagger.h"

namespace DynExpInstr
{
	class QutoolsQuTAGStandard;
	class QutoolsQuTAGStandardData;

	namespace QutoolsQuTAGStandardTasks
	{
		void UpdateStreamMode(Util::SynchronizedPointer<QutoolsQuTAGStandardData>& InstrData);

		class InitTask : public TimeTaggerTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<TimeTaggerTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public TimeTaggerTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<TimeTaggerTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public TimeTaggerTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<TimeTaggerTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ReadDataTask final : public DynExp::TaskBase
		{
		public:
			ReadDataTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetStreamSizeTask final : public DynExp::TaskBase
		{
		public:
			SetStreamSizeTask(size_t BufferSizeInSamples, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), BufferSizeInSamples(BufferSizeInSamples) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const size_t BufferSizeInSamples;
		};

		class ClearTask final : public DynExp::TaskBase
		{
		public:
			ClearTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ConfigureInputTask final : public DynExp::TaskBase
		{
		public:
			ConfigureInputTask(bool UseRisingEdge, double ThresholdInVolts, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), UseRisingEdge(UseRisingEdge), ThresholdInVolts(ThresholdInVolts) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const bool UseRisingEdge;
			const double ThresholdInVolts;
		};

		class SetExposureTimeTask final : public DynExp::TaskBase
		{
		public:
			SetExposureTimeTask(Util::picoseconds ExposureTime, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), ExposureTime(ExposureTime) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const Util::picoseconds ExposureTime;
		};

		class SetCoincidenceWindowTask final : public DynExp::TaskBase
		{
		public:
			SetCoincidenceWindowTask(Util::picoseconds CoincidenceWindow, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), CoincidenceWindow(CoincidenceWindow) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const Util::picoseconds CoincidenceWindow;
		};

		class SetDelayTask final : public DynExp::TaskBase
		{
		public:
			SetDelayTask(Util::picoseconds Delay, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Delay(Delay) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const Util::picoseconds Delay;
		};

		class SetHBTActiveTask final : public DynExp::TaskBase
		{
		public:
			SetHBTActiveTask(bool Enable, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Enable(Enable) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const bool Enable;
		};

		class ConfigureHBTTask final : public DynExp::TaskBase
		{
		public:
			ConfigureHBTTask(Util::picoseconds BinWidth, size_t BinCount, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), BinWidth(BinWidth), BinCount(BinCount) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const Util::picoseconds BinWidth;
			const size_t BinCount;
		};

		class ResetHBTTask final : public DynExp::TaskBase
		{
		public:
			ResetHBTTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class QutoolsQuTAGStandardData : public TimeTaggerData
	{
	public:
		QutoolsQuTAGStandardData() : Channel(0) {}
		virtual ~QutoolsQuTAGStandardData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::QutoolsStandardTDCHardwareAdapter> HardwareAdapter;

		auto GetChannel() const noexcept { return Channel; }
		void SetChannel(DynExpHardware::QutoolsStandardTDCHardwareAdapter::ChannelType Channel) noexcept { this->Channel = Channel; }

	private:
		void ResetImpl(dispatch_tag<TimeTaggerData>) override final;
		virtual void ResetImpl(dispatch_tag<QutoolsQuTAGStandardData>) {};

		DynExpHardware::QutoolsStandardTDCHardwareAdapter::ChannelType Channel;
	};

	class QutoolsQuTAGStandardParams : public TimeTaggerParams
	{
	public:
		QutoolsQuTAGStandardParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : TimeTaggerParams(ID, Core) {}
		virtual ~QutoolsQuTAGStandardParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "QutoolsQuTAGStandardParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::QutoolsStandardTDCHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "qutools TDC device", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::NumberType> Channel = { *this, "Channel", "Channel",
			"Channel of the quTAG device this instrument refers to", true, 0, 0, std::numeric_limits<DynExpHardware::QutoolsStandardTDCHardwareAdapter::ChannelType>::max(), 1, 0 };
		Param<ParamsConfigDialog::NumberType> CrossCorrChannel = { *this, "CrossCorrChannel", "Correlation channel",
			"Channel of the quTAG device to compute correlations (e.g. g(2)) with", false, 1, 0, std::numeric_limits<DynExpHardware::QutoolsStandardTDCHardwareAdapter::ChannelType>::max(), 1, 0 };

	private:
		void ConfigureParamsImpl(dispatch_tag<TimeTaggerParams>) override final { ConfigureParamsImpl(dispatch_tag<QutoolsQuTAGStandardParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<QutoolsQuTAGStandardParams>) {}
	};

	class QutoolsQuTAGStandardConfigurator : public TimeTaggerConfigurator
	{
	public:
		using ObjectType = QutoolsQuTAGStandard;
		using ParamsType = QutoolsQuTAGStandardParams;

		QutoolsQuTAGStandardConfigurator() = default;
		virtual ~QutoolsQuTAGStandardConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<QutoolsQuTAGStandardConfigurator>(ID, Core); }
	};

	class QutoolsQuTAGStandard : public TimeTagger
	{
	public:
		using ParamsType = QutoolsQuTAGStandardParams;
		using ConfigType = QutoolsQuTAGStandardConfigurator;
		using InstrumentDataType = QutoolsQuTAGStandardData;

		constexpr static auto Name() noexcept { return "qutools quTAG"; }

		QutoolsQuTAGStandard(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~QutoolsQuTAGStandard() {}

		virtual std::string GetName() const override { return Name(); }

		virtual double GetMinThresholdInVolts() const noexcept override { return -2; }
		virtual double GetMaxThresholdInVolts() const noexcept override { return 3; }

		virtual Util::picoseconds GetResolution() const override;
		virtual size_t GetBufferSize() const override;

		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::ReadDataTask>(CallbackFunc); }
		virtual void SetStreamSize(size_t BufferSizeInSamples, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::SetStreamSizeTask>(BufferSizeInSamples, CallbackFunc); }
		virtual void Clear(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::ClearTask>(CallbackFunc); }
		virtual void ConfigureInput(bool UseRisingEdge, double ThresholdInVolts, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::ConfigureInputTask>(UseRisingEdge, ThresholdInVolts, CallbackFunc); }
		virtual void SetExposureTime(Util::picoseconds ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::SetExposureTimeTask>(ExposureTime, CallbackFunc); }
		virtual void SetCoincidenceWindow(Util::picoseconds CoincidenceWindow, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::SetCoincidenceWindowTask>(CoincidenceWindow, CallbackFunc); }
		virtual void SetDelay(Util::picoseconds Delay, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::SetDelayTask>(Delay, CallbackFunc); }
		virtual void SetHBTActive(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::SetHBTActiveTask>(Enable, CallbackFunc); }
		virtual void ConfigureHBT(Util::picoseconds BinWidth, size_t BinCount, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::ConfigureHBTTask>(BinWidth, BinCount, CallbackFunc); }
		virtual void ResetHBT(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<QutoolsQuTAGStandardTasks::ResetHBTTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<TimeTagger>) override final;
		virtual void ResetImpl(dispatch_tag<QutoolsQuTAGStandard>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<QutoolsQuTAGStandardTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<QutoolsQuTAGStandardTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<QutoolsQuTAGStandardTasks::UpdateTask>(); }
	};
}