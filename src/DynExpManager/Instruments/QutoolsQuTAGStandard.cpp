// This file is part of DynExp.

#include "stdafx.h"
#include "QutoolsQuTAGStandard.h"

namespace DynExpInstr
{
	void QutoolsQuTAGStandardTasks::UpdateStreamMode(Util::SynchronizedPointer<QutoolsQuTAGStandardData>& InstrData)
	{
		// In case one is only interested in the counts per exposure time window, there is no need to transfer all
		// single count events to the computer. So, mute the channel in that case.
		if (InstrData->GetStreamMode() == TimeTaggerData::StreamModeType::Counts)
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel(), DynExpHardware::QutoolsTDCSyms::TDC_FilterType::FILTER_MUTE);
		else
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel());

		InstrData->ClearStreamModeChanged();
	}

	void QutoolsQuTAGStandardTasks::InitTask::InitFuncImpl(dispatch_tag<TimeTaggerTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<QutoolsQuTAGStandard>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->SetChannel(InstrParams->Channel);

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		UpdateStreamMode(InstrData);
		InstrData->HardwareAdapter->EnableChannel(InstrData->GetChannel());

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void QutoolsQuTAGStandardTasks::ExitTask::ExitFuncImpl(dispatch_tag<TimeTaggerTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DisableChannel(InstrData->GetChannel());
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void QutoolsQuTAGStandardTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<TimeTaggerTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

			if (InstrData->GetStreamModeChanged())
				UpdateStreamMode(InstrData);
		} // InstrData unlocked here.

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::ReadDataTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());
		auto SampleStream = InstrData->GetCastSampleStream<TimeTaggerData::SampleStreamType>();

		if (InstrData->GetStreamMode() == TimeTaggerData::StreamModeType::Counts)
		{
			auto Counts = InstrData->HardwareAdapter->GetCoincidenceCounts(InstrData->GetChannel() + 1);
			if (Counts.second)
				SampleStream->WriteSample(Counts.first);
		}
		else
			SampleStream->WriteSamples(InstrData->HardwareAdapter->GetTimestamps(InstrData->GetChannel()));

		if (InstrData->GetHBTResults().Enabled)
		{
			InstrData->GetHBTResults().EventCounts = InstrData->HardwareAdapter->GetHBTEventCounts();
			InstrData->GetHBTResults().IntegrationTime = InstrData->HardwareAdapter->GetHBTIntegrationTime();
			InstrData->GetHBTResults().ResultVector = InstrData->HardwareAdapter->GetHBTResult();
		}

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::SetStreamSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetTimestampBufferSize(Util::NumToT<DynExpHardware::QutoolsTDCSyms::Int32>(BufferSizeInSamples));
		InstrData->GetSampleStream()->SetStreamSize(BufferSizeInSamples);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::ClearTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ClearTimestamps(InstrData->GetChannel());

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::ConfigureInputTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ConfigureSignalConditioning(InstrData->GetChannel(), DynExpHardware::QutoolsTDCSyms::SCOND_MISC, UseRisingEdge, ThresholdInVolts);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::SetExposureTimeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetExposureTime(std::chrono::duration_cast<std::chrono::milliseconds>(ExposureTime));

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::SetCoincidenceWindowTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetCoincidenceWindow(CoincidenceWindow);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::SetDelayTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetChannelDelay(InstrData->GetChannel(), Delay);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::SetHBTActiveTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<QutoolsQuTAGStandard>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->EnableHBT(Enable);
		InstrData->GetHBTResults().Enabled = Enable;

		if (Enable)
		{
			InstrData->HardwareAdapter->ConfigureHBTChannels(InstrData->GetChannel(), InstrParams->CrossCorrChannel);

			// Disable filters for HBT.
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel());
			InstrData->HardwareAdapter->ConfigureFilter(InstrParams->CrossCorrChannel);
			
			// Avoid going back to pre-defined stream mode if the flag was set and not cleared before.
			InstrData->ClearStreamModeChanged();
		}

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::ConfigureHBTTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		if (BinWidth / InstrData->HardwareAdapter->GetTimebase() < 1 || BinWidth / InstrData->HardwareAdapter->GetTimebase() > 1e6)
			throw Util::OutOfRangeException("The bin width exceeds the allowed range (see tdchbt.h's reference).");
		if (BinCount < 16 || BinCount > 64000)
			throw Util::OutOfRangeException("The number of bins exceeds the allowed range (see tdchbt.h's reference).");

		InstrData->HardwareAdapter->ConfigureHBTParams(BinWidth, Util::NumToT<DynExpHardware::QutoolsTDCSyms::Int32>(BinCount));

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGStandardTasks::ResetHBTTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(Instance.InstrumentDataGetter());

		if (InstrData->GetHBTResults().Enabled)
			InstrData->HardwareAdapter->ResetHBT();

		return {};
	}

	void QutoolsQuTAGStandardData::ResetImpl(dispatch_tag<TimeTaggerData>)
	{
		Channel = 0;

		ResetImpl(dispatch_tag<QutoolsQuTAGStandardData>());
	}

	QutoolsQuTAGStandard::QutoolsQuTAGStandard(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: TimeTagger(OwnerThreadID, std::move(Params))
	{
	}

	Util::picoseconds QutoolsQuTAGStandard::GetResolution() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(GetInstrumentData());

		return InstrData->HardwareAdapter->GetTimebase();
	}

	size_t QutoolsQuTAGStandard::GetBufferSize() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAGStandard>(GetInstrumentData());

		return InstrData->HardwareAdapter->GetBufferSize();
	}

	void QutoolsQuTAGStandard::ResetImpl(dispatch_tag<TimeTagger>)
	{
		ResetImpl(dispatch_tag<QutoolsQuTAGStandard>());
	}
}