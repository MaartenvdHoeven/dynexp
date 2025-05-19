// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterQutoolsStandardTDC.h"

namespace DynExpHardware
{
	// Returns the serial numbers of all available qutool TDC devices as a std::vector
	// sorted by device number in ascending order.
	auto QutoolsStandardTDCHardwareAdapter::Enumerate()
	{
		unsigned int DeviceCount{};
		auto Result = QutoolsTDCSyms::TDC_discover(&DeviceCount);
		if (Result != TDC_Ok)
			throw QutoolsTDCException("Error enumerating qutools TDC devices.", Result);

		std::vector<std::string> DeviceDescriptors;
		for (decltype(DeviceCount) i = 0; i < DeviceCount; ++i)
		{
			std::string DeviceName;
			DeviceName.resize(32);	// Must be at least 16 bytes (given by documentation of TDC_getDeviceInfo()).

			Result = QutoolsTDCSyms::TDC_getDeviceInfo(i, nullptr, nullptr, DeviceName.data(), nullptr);
			if (Result != TDC_Ok)
				throw QutoolsTDCException("Error obtaining TDC device information.", Result);
			DeviceName = Util::TrimTrailingZeros(DeviceName);

			DeviceDescriptors.emplace_back(std::move(DeviceName));
		}

		return DeviceDescriptors;
	}

	Util::TextValueListType<QutoolsStandardTDCHardwareAdapterParams::EdgeType> QutoolsStandardTDCHardwareAdapterParams::EdgeTypeStrList()
	{
		Util::TextValueListType<EdgeType> List = {
			{ "Trigger on rising edge.", EdgeType::RisingEdge },
			{ "Trigger on falling edge.", EdgeType::FallingEdge }
		};

		return List;
	}

	void QutoolsStandardTDCHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
		auto QutoolsTDCDevices = QutoolsStandardTDCHardwareAdapter::Enumerate();
		if (!DeviceDescriptor.Get().empty() &&
			std::find(QutoolsTDCDevices.cbegin(), QutoolsTDCDevices.cend(), DeviceDescriptor) == std::cend(QutoolsTDCDevices))
			QutoolsTDCDevices.push_back(DeviceDescriptor);
		if (QutoolsTDCDevices.empty())
			throw Util::EmptyException("There is not any available qutools TDC device.");
		DeviceDescriptor.SetTextList(std::move(QutoolsTDCDevices));

		ConfigureParamsImpl(dispatch_tag<QutoolsStandardTDCHardwareAdapterParams>());
	}

	QutoolsStandardTDCHardwareAdapter::QutoolsTDCSynchronizer::LockType QutoolsStandardTDCHardwareAdapter::QutoolsTDCSynchronizer::Lock(
		const std::chrono::milliseconds Timeout)
	{
		return GetInstance().AcquireLock(Timeout);
	}

	QutoolsStandardTDCHardwareAdapter::QutoolsTDCSynchronizer& QutoolsStandardTDCHardwareAdapter::QutoolsTDCSynchronizer::GetInstance() noexcept
	{
		static QutoolsTDCSynchronizer Instance;

		return Instance;
	}

	QutoolsStandardTDCHardwareAdapter::CoincidenceDataType::CoincidenceDataType(CountsType&& Counts, QutoolsTDCSyms::Int32 NumUpdates)
		: Counts(std::move(Counts)), NumUpdates(NumUpdates)
	{
		HasBeenRead.resize(this->Counts.size(), false);
	}

	QutoolsStandardTDCHardwareAdapter::QutoolsStandardTDCHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	QutoolsStandardTDCHardwareAdapter::~QutoolsStandardTDCHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
	}

	void QutoolsStandardTDCHardwareAdapter::EnableChannels(bool EnableStartChannel, QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		EnableChannelsUnsafe(ChannelMask);
	}

	void QutoolsStandardTDCHardwareAdapter::EnableChannel(ChannelType Channel) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		if (Channel >= ChannelCount)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and " + Util::ToStr(ChannelCount))));

		auto CurrentState = GetEnabledChannelsUnsafe();
		EnableChannelsUnsafe(CurrentState.second | (1 << Channel));
	}

	void QutoolsStandardTDCHardwareAdapter::DisableChannel(ChannelType Channel) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		if (Channel >= ChannelCount)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and " + Util::ToStr(ChannelCount))));

		auto CurrentState = GetEnabledChannelsUnsafe();
		EnableChannelsUnsafe(CurrentState.second & ~(1 << Channel));
	}

	void QutoolsStandardTDCHardwareAdapter::SetExposureTime(std::chrono::milliseconds ExposureTime) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetExposureTimeUnsafe(ExposureTime);
	}

	void QutoolsStandardTDCHardwareAdapter::SetCoincidenceWindow(ValueType CoincidenceWindow) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetCoincidenceWindowUnsafe(CoincidenceWindow);
	}

	void QutoolsStandardTDCHardwareAdapter::SetChannelDelay(ChannelType Channel, Util::picoseconds ChannelDelay) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetChannelDelayUnsafe(Channel, ChannelDelay);
	}

	void QutoolsStandardTDCHardwareAdapter::SetTimestampBufferSize(QutoolsTDCSyms::Int32 Size) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetTimestampBufferSizeUnsafe(Size);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureSignalConditioning(ChannelType Channel,
		QutoolsTDCSyms::TDC_SignalCond Conditioning, bool UseRisingEdge, double ThresholdInVolts) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureSignalConditioningUnsafe(Channel, Conditioning, UseRisingEdge, ThresholdInVolts);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureFilter(ChannelType Channel, QutoolsTDCSyms::TDC_FilterType FilterType,
		QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureFilterUnsafe(Channel, FilterType, ChannelMask);
	}

	void QutoolsStandardTDCHardwareAdapter::EnableHBT(bool Enable) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		EnableHBTUnsafe(Enable);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureHBTChannels(ChannelType FirstChannel, ChannelType SecondChannel) const
	{
		if (FirstChannel >= 5 || SecondChannel >= 5) // DIFFERENCE TO MC: Only channels 0 to 5.
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify valid channels between 0 and 5.")));

		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureHBTChannelsUnsafe(FirstChannel, SecondChannel);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureHBTParams(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureHBTParamsUnsafe(BinWidth, BinCount);
	}

	void QutoolsStandardTDCHardwareAdapter::ResetHBT() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ResetHBTUnsafe();
	}

	QutoolsTDCSyms::Int64 QutoolsStandardTDCHardwareAdapter::GetHBTEventCounts() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTEventCountsUnsafe();
	}

	std::chrono::microseconds QutoolsStandardTDCHardwareAdapter::GetHBTIntegrationTime() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTIntegrationTimeUnsafe();
	}

	QutoolsStandardTDCHardwareAdapter::HBTResultsType QutoolsStandardTDCHardwareAdapter::GetHBTResult() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTResultUnsafe();
	}

	QutoolsStandardTDCHardwareAdapter::ValueType QutoolsStandardTDCHardwareAdapter::GetTimebase() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return Timebase;
	}

	QutoolsTDCSyms::Int32 QutoolsStandardTDCHardwareAdapter::GetBufferSize() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return BufferSize;
	}

	// Extracts timestamp series for specific channel.
	std::vector<QutoolsStandardTDCHardwareAdapter::ValueType> QutoolsStandardTDCHardwareAdapter::GetTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadTimestampsUnsafe();

		auto Timestamps = TimestampsPerChannel.extract(Channel);
		return !Timestamps.empty() ? std::move(Timestamps.mapped()) : std::vector<QutoolsStandardTDCHardwareAdapter::ValueType>();
	}

	// Just sums up timestamp series for specific channel while keeping the series.
	size_t QutoolsStandardTDCHardwareAdapter::GetCountsFromTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadTimestampsUnsafe();

		return TimestampsPerChannel[Channel].size();
	}

	const QutoolsStandardTDCHardwareAdapter::CoincidenceDataType& QutoolsStandardTDCHardwareAdapter::GetCoincidenceCounts() const // DIFFERENCE TO MC: The first input of the function TDC_getCoincCounters can have less entries, since the number of channels is lower. 
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		// See documentation for TDC_getCoincCounters()
		std::vector<QutoolsTDCSyms::Int32> Counts(59);  // The array must have 31 elements: 0(5), 1, 2, 3, 4, 1/2, 1/3, 2/3, 1/4, 2/4, 3/4, 1/5, 2/5, 3/5, 4/5, 1/2/3, 1/2/4, 1/3/4, 2/3/4, 1/2/5, 1/3/5, 2/3/5, 1/4/5, 2/4/5, 3/4/5, 1/2/3/4, 1/2/3/5, 1/2/4/5, 1/3/4/5, 2/3/4/5, 1/2/3/4/5:
		QutoolsTDCSyms::Int32 NumUpdates{};

		auto Result = QutoolsTDCSyms::TDC_getCoincCounters(Counts.data(), &NumUpdates);
		CheckError(Result);

		if (NumUpdates)
			CoincidenceData = { std::move(Counts), NumUpdates };

		return CoincidenceData;
	}

	// First of pair denotes the coincidences of the specified channel (combination), second of pair denotes the number
	// of updates the qutools TDC device has made since the last call to TDC_getCoincCounters().
	std::pair<QutoolsTDCSyms::Int32, QutoolsTDCSyms::Int32> QutoolsStandardTDCHardwareAdapter::GetCoincidenceCounts(ChannelType Channel) const
	{
		// See documentation for TDC_getCoincCounters()
		if (Channel < 0 || Channel >= 59)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and 59.")));

		GetCoincidenceCounts();

		// Just after CoincidenceData has been default-constructed, CoincidenceData.Counts is empty.
		if (CoincidenceData.Counts.empty() || CoincidenceData.Counts.size() != CoincidenceData.HasBeenRead.size() || CoincidenceData.Counts.size() <= Channel)
			return { 0, 0 };

		auto RetVal = std::make_pair(CoincidenceData.Counts[Channel], CoincidenceData.HasBeenRead[Channel] ? 0 : CoincidenceData.NumUpdates);
		CoincidenceData.HasBeenRead[Channel] = true;

		return RetVal;
	}

	void QutoolsStandardTDCHardwareAdapter::ClearTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		TimestampsPerChannel[Channel].clear();
	}

	void QutoolsStandardTDCHardwareAdapter::Init()
	{
		DeviceConnected = false;
		DeviceNumber = std::numeric_limits<decltype(DeviceNumber)>::max();
		Timebase = ValueType(-1);
		BufferSize = 0;
		ChannelCount = 0;
		TimestampsPerChannel.clear();
		CoincidenceData = {};
	}

	void QutoolsStandardTDCHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();
		Init();

		ResetImpl(dispatch_tag<QutoolsStandardTDCHardwareAdapter>());
	}

	void QutoolsStandardTDCHardwareAdapter::EnsureReadyStateChild()
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	bool QutoolsStandardTDCHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpened();
	}

	bool QutoolsStandardTDCHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void QutoolsStandardTDCHardwareAdapter::CheckError(const int Result, const std::source_location Location) const
	{
		if (Result == TDC_Ok)
			return;

		std::string ErrorString(QutoolsTDCSyms::TDC_perror(Result));

		// AcquireLock() has already been called by an (in)direct caller of this function.
		ThrowExceptionUnsafe(std::make_exception_ptr(QutoolsTDCException(std::move(ErrorString), Result, Location)));
	}

	void QutoolsStandardTDCHardwareAdapter::OpenUnsafe()
	{
		if (IsOpened())
			return;

		std::ptrdiff_t DeviceNumber{ -1 };
		QutoolsTDCSyms::Int32 TimestampBufferSize{};
		bool UseRisingEdge{};
		double ThresholdInVolts{};
		std::chrono::milliseconds ExposureTime{};
		ValueType CoincidenceWindow{};

		{
			auto DerivedParams = dynamic_Params_cast<QutoolsStandardTDCHardwareAdapter>(GetParams());

			auto DeviceSerials = Enumerate();
			auto DeviceSerialIt = std::find(DeviceSerials.cbegin(), DeviceSerials.cend(), DerivedParams->DeviceDescriptor.Get());
			DeviceNumber = std::distance(DeviceSerials.cbegin(), DeviceSerialIt);

			if (DeviceNumber < 0 || Util::NumToT<size_t>(DeviceNumber) >= DeviceSerials.size())
			{
				std::string Serial = DeviceSerialIt == DeviceSerials.cend() ? "< unknown >" : *DeviceSerialIt;

				ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotFoundException(
					"The qutools TDC device with serial " + Serial + " has not been found.")));
			}

			TimestampBufferSize = DerivedParams->DefaultTimestampBufferSize;
			UseRisingEdge = DerivedParams->DefaultTriggerEdge == QutoolsStandardTDCHardwareAdapterParams::EdgeType::RisingEdge;
			ThresholdInVolts = DerivedParams->DefaultThresholdInVolts;
			ExposureTime = std::chrono::milliseconds(DerivedParams->DefaultExposureTime);
			CoincidenceWindow = ValueType(DerivedParams->DefaultCoincidenceWindow);
		} // DerivedParams unlocked here.

		this->DeviceNumber = DeviceNumber;
		auto Result = QutoolsTDCSyms::TDC_connect(this->DeviceNumber);
		CheckError(Result);
		DeviceConnected = true;

		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		double Timebase{};	// in s
		Result = QutoolsTDCSyms::TDC_getTimebase(&Timebase);
		CheckError(Result);
		this->Timebase = ValueType(Timebase * 1e12);

		QutoolsTDCSyms::Int32 BufferSize{};
		Result = QutoolsTDCSyms::TDC_getTimestampBufferSize(&BufferSize);
		CheckError(Result);
		this->BufferSize = BufferSize;

		ChannelCount = 5; // Q: Number of channels (including the start input) has to be hard-coded here, since this function does not exist for quTAG standard.

		SetTimestampBufferSizeUnsafe(TimestampBufferSize);
		SetExposureTimeUnsafe(ExposureTime);
		SetCoincidenceWindowUnsafe(CoincidenceWindow);
		for (decltype(ChannelCount) i = 0; i < ChannelCount; ++i)
			ConfigureSignalConditioningUnsafe(i, QutoolsTDCSyms::SCOND_MISC, UseRisingEdge, ThresholdInVolts);
	}

	void QutoolsStandardTDCHardwareAdapter::CloseUnsafe()
	{
		if (IsOpened())
		{
			// Handle now considered invalid, even if TDC_disconnect() fails.
			DeviceConnected = false;

			auto Result = QutoolsTDCSyms::TDC_disconnect(DeviceNumber);
			CheckError(Result);
		}
	}

	void QutoolsStandardTDCHardwareAdapter::AddressThisTDCDeviceUnsafe() const
	{
		auto Result = QutoolsTDCSyms::TDC_addressDevice(DeviceNumber);
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::ReadTimestampsUnsafe() const
	{
		std::vector<QutoolsTDCSyms::Int64> Timestamps(BufferSize);
		std::vector<ChannelType> Channels(BufferSize);
		QutoolsTDCSyms::Int32 NumValid{};

		{
			auto TDCLock = QutoolsTDCSynchronizer::Lock();
			AddressThisTDCDeviceUnsafe();

			auto Result = QutoolsTDCSyms::TDC_getLastTimestamps(true, Timestamps.data(), Channels.data(), &NumValid); // DIFFERENCE TO MC: Channels only goes from 0 to 7 (instead of 31).
			CheckError(Result);
		} // TDCLock unlocked here.

		if (Timestamps.size() != Channels.size() || NumValid < 0 || NumValid > Timestamps.size())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
				"Received invalid data from TDC_getLastTimestamps().")));

		for (size_t i = 0; i < NumValid; ++i)
			TimestampsPerChannel[Channels[i]].push_back(ValueType(Timestamps[i]));
	}

	// QutoolsTDCSynchronizer::Lock() and AddressThisTDCDeviceUnsafe() must be called manually before calling this function!
	void QutoolsStandardTDCHardwareAdapter::EnableChannelsUnsafe(QutoolsTDCSyms::Int32 ChannelMask) const // DIFFERENCE TO MC: The quTAG Standard treats the start channel as channel 0.
	{
		auto Result = QutoolsTDCSyms::TDC_enableChannels(ChannelMask);  // E.g. ChannelMask = 31 means that start (channel 0) and all channels 1-4 are enabled.
		CheckError(Result);
	}

	// QutoolsTDCSynchronizer::Lock() and AddressThisTDCDeviceUnsafe() must be called manually before calling this function!
	// First of pair denotes whether the start channel is enabled, second of pair denotes the mask of enabled channels.
	std::pair<bool, QutoolsTDCSyms::Int32> QutoolsStandardTDCHardwareAdapter::GetEnabledChannelsUnsafe() const // DIFFERENCE TO MC: The quTAG Standard treats the start channel as channel 0.
	{
		QutoolsTDCSyms::Bln32 StartEnabled{};
		QutoolsTDCSyms::Int32 ChannelMask{};

		auto Result = QutoolsTDCSyms::TDC_getChannelsEnabled(&ChannelMask);
		CheckError(Result);

		StartEnabled = (ChannelMask & 0x01) != 0;
		return std::make_pair(static_cast<bool>(StartEnabled), ChannelMask);
	}

	void QutoolsStandardTDCHardwareAdapter::SetExposureTimeUnsafe(std::chrono::milliseconds ExposureTime) const
	{
		auto Result = QutoolsTDCSyms::TDC_setExposureTime(Util::NumToT<QutoolsTDCSyms::Int32>(ExposureTime.count()));
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::SetCoincidenceWindowUnsafe(ValueType CoincidenceWindow) const
	{
		auto Result = QutoolsTDCSyms::TDC_setCoincidenceWindow(Util::NumToT<QutoolsTDCSyms::Int32>(CoincidenceWindow / Timebase));
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::SetChannelDelayUnsafe(ChannelType Channel, Util::picoseconds ChannelDelay) const // DIFFERENCE TO MC: The function TDC_setChannelDelay does not exist for the quTAG Standard.
	{
		//auto Result = QutoolsTDCSyms::TDC_setChannelDelay(Channel, Util::NumToT<QutoolsTDCSyms::Int32>(ChannelDelay.count()));
		auto Result = 0;
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::SetTimestampBufferSizeUnsafe(QutoolsTDCSyms::Int32 Size) const
	{
		auto Result = QutoolsTDCSyms::TDC_setTimestampBufferSize(Size);
		CheckError(Result);

		BufferSize = Size;
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureSignalConditioningUnsafe(ChannelType Channel,
		QutoolsTDCSyms::TDC_SignalCond Conditioning, bool UseRisingEdge, double ThresholdInVolts) const
	{
		auto Result = QutoolsTDCSyms::TDC_configureSignalConditioning(Channel, Conditioning, UseRisingEdge, ThresholdInVolts); // channel 0 = start as for MC?
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureFilterUnsafe(ChannelType Channel,
		QutoolsTDCSyms::TDC_FilterType FilterType, QutoolsTDCSyms::Int32 ChannelMask) const // DIFFERENCE TO MC: Setting the filters is a monetary feature. For our T010175, e.g. we did not buy it.
	{
		// auto Result = QutoolsTDCSyms::TDC_configureFilter(Channel + 1, FilterType, ChannelMask); // If the feature is not available, the output is always 14 (Requested feature is not available.).
		auto Result = 0;
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::EnableHBTUnsafe(bool Enable) const
	{
		auto Result = QutoolsTDCSyms::TDC_enableHbt(Enable);
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureHBTChannelsUnsafe(ChannelType FirstChannel, ChannelType SecondChannel) const
	{
		auto Result = QutoolsTDCSyms::TDC_setHbtInput(FirstChannel + 1, SecondChannel + 1);
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::ConfigureHBTParamsUnsafe(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const
	{
		auto Result = QutoolsTDCSyms::TDC_setHbtParams(Util::NumToT<QutoolsTDCSyms::Int32>(BinWidth / Timebase), BinCount);
		CheckError(Result);
	}

	void QutoolsStandardTDCHardwareAdapter::ResetHBTUnsafe() const
	{
		auto Result = QutoolsTDCSyms::TDC_resetHbtCorrelations();
		CheckError(Result);
	}

	QutoolsTDCSyms::Int64 QutoolsStandardTDCHardwareAdapter::GetHBTEventCountsUnsafe() const
	{
		QutoolsTDCSyms::Int64 TotalCount{}, LastCount{};
		double LastRate{};

		auto Result = QutoolsTDCSyms::TDC_getHbtEventCount(&TotalCount, &LastCount, &LastRate);
		CheckError(Result);

		return TotalCount;
	}

	std::chrono::microseconds QutoolsStandardTDCHardwareAdapter::GetHBTIntegrationTimeUnsafe() const
	{
		double IntegrationTime{};

		auto Result = QutoolsTDCSyms::TDC_getHbtIntegrationTime(&IntegrationTime);
		CheckError(Result);

		return std::chrono::microseconds(static_cast<std::chrono::microseconds::rep>(std::round(IntegrationTime * std::micro::den)));
	}

	QutoolsStandardTDCHardwareAdapter::HBTResultsType QutoolsStandardTDCHardwareAdapter::GetHBTResultUnsafe() const
	{
		auto HBTFunc = QutoolsTDCSyms::TDC_createHbtFunction();
		if (!HBTFunc)
			CheckError(TDC_Error);

		try
		{
			auto Result = QutoolsTDCSyms::TDC_calcHbtG2(HBTFunc);
			CheckError(Result);

			HBTResultsType HBTResult;
			for (decltype(HBTFunc->size) i = 0; i < HBTFunc->size; ++i)
				HBTResult.emplace_back(HBTFunc->values[i], ((i - HBTFunc->indexOffset) * Timebase * HBTFunc->binWidth).count() / std::pico::den);

			QutoolsTDCSyms::TDC_releaseHbtFunction(HBTFunc);
			HBTFunc = nullptr;

			return HBTResult;
		}
		catch (...)
		{
			if (HBTFunc)
				QutoolsTDCSyms::TDC_releaseHbtFunction(HBTFunc);

			throw;
		}
	}
}
