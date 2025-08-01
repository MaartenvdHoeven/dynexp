// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterStanda8SMC5.h"

namespace DynExpHardware
{
	std::vector<std::string> StandaHardwareAdapter::Enumerate()
	{
		std::vector<std::string> devices;

		StandaSyms::device_enumeration_t device_enum = StandaSyms::enumerate_devices(ENUMERATE_PROBE, nullptr);
		if (!device_enum) 
			return devices;

		const int count = StandaSyms::get_device_count(device_enum);
		for (int i = 0; i < count; ++i) {
			const char* name = StandaSyms::get_device_name(device_enum, i);
			if (name) devices.emplace_back(name);
		}

		StandaSyms::free_enumerate_devices(device_enum);
		return devices;
	}

	void StandaHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
	//	auto StandaDevices = StandaHardwareAdapter::Enumerate();
	//	if (!DeviceDescriptor.Get().empty() &&
	//		std::find(StandaDevices.cbegin(), StandaDevices.cend(), DeviceDescriptor) == std::cend(StandaDevices))
	//		StandaDevices.push_back(DeviceDescriptor);
	//	if (StandaDevices.empty())
	//		throw Util::EmptyException("There is not any available Standa controller.");
	//	DeviceDescriptor.SetTextList(std::move(StandaDevices));

	//	ConfigureParamsImpl(dispatch_tag<StandaHardwareAdapterParams>());
	}

	StandaHardwareAdapter::StandaHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	StandaHardwareAdapter::~StandaHardwareAdapter()
	{
		CloseUnsafe();
	}

	void StandaHardwareAdapter::Init()
	{
	//	auto DerivedParams = dynamic_Params_cast<StandaHardwareAdapter>(GetParams());

	//	DeviceDescriptor = DerivedParams->DeviceDescriptor.Get();

	//	StandaHandleValid = false;
	//	StandaHandle = StandaSyms::SA_CTL_DeviceHandle_t();

		// For MWE, assume static dummy descriptor
		DeviceHandle = device_undefined;
	}

	void StandaHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		CloseUnsafe();
		Init();  // Could call OpenUnsafe() here later if needed
	}

	void StandaHardwareAdapter::EnsureReadyStateChild()
	{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	OpenUnsafe();
	}

	bool StandaHardwareAdapter::IsReadyChild() const
	{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	auto Exception = GetExceptionUnsafe();
	//	Util::ForwardException(Exception);

		return IsOpened();
	}

	bool StandaHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void StandaHardwareAdapter::CheckError(const StandaSyms::result_t Result, const std::source_location Location) const
	{
		if (Result != result_ok) {
			throw StandaException("XIMC error", Result);
		}
	}

	void StandaHardwareAdapter::OpenUnsafe()
	{
	//	if (IsOpened())
	//		return;

	//	auto Result = StandaSyms::SA_CTL_Open(&StandaHandle, DeviceDescriptor.c_str(), "");
	//	CheckError(Result);

	//	StandaHandleValid = true;
	}

	void StandaHardwareAdapter::CloseUnsafe()
	{
		if (DeviceHandle != device_undefined) {
			// Normally: close_device(&DeviceHandle);
			DeviceHandle = device_undefined;
		}
	}

	//StandaHardwareAdapter::PositionType StandaHardwareAdapter::GetCurrentPosition(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	PositionType Position;
	//	auto Result = StandaSyms::SA_CTL_GetProperty_i64(StandaHandle, Channel, SA_CTL_PKEY_POSITION, &Position, nullptr);
	//	CheckError(Result);

	//	return Position;
	//}

	//StandaHardwareAdapter::PositionType StandaHardwareAdapter::GetTargetPosition(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	PositionType TargetPosition;
	//	auto Result = StandaSyms::SA_CTL_GetProperty_i64(StandaHandle, Channel, SA_CTL_PKEY_TARGET_POSITION, &TargetPosition, nullptr);
	//	CheckError(Result);

	//	return TargetPosition;
	//}

	//StandaHardwareAdapter::PositionType StandaHardwareAdapter::GetVelocity(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	PositionType Velocity;
	//	auto Result = StandaSyms::SA_CTL_GetProperty_i64(StandaHandle, Channel, SA_CTL_PKEY_MOVE_VELOCITY, &Velocity, nullptr);
	//	CheckError(Result);

	//	return Velocity;
	//}

	//int32_t StandaHardwareAdapter::GetChannelState(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	int32_t State;
	//	auto Result = StandaSyms::SA_CTL_GetProperty_i32(StandaHandle, Channel, SA_CTL_PKEY_CHANNEL_STATE, &State, 0);
	//	CheckError(Result);

	//	return State;
	//}

	//void StandaHardwareAdapter::Calibrate(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	auto Result = StandaSyms::SA_CTL_Calibrate(StandaHandle, Channel, 0);
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::Reference(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	// Using direction configured as 'Safe Direction' on the Standa controller. Not allowing
	//	// to change this setting, because this requires recalibration.
	//	auto Result = StandaSyms::SA_CTL_Reference(StandaHandle, Channel, 0);
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::SetHoldTime(const ChannelType Channel, const std::chrono::milliseconds HoldTime) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	StandaSyms::SA_CTL_Result_t Result;
	//	if (HoldTime.count() < 0)
	//		Result = StandaSyms::SA_CTL_SetProperty_i32(StandaHandle, Channel, SA_CTL_PKEY_HOLD_TIME, SA_CTL_HOLD_TIME_INFINITE);
	//	else
	//		Result = StandaSyms::SA_CTL_SetProperty_i32(StandaHandle, Channel, SA_CTL_PKEY_HOLD_TIME, Util::NumToT<int32_t>(HoldTime.count()));
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::SetVelocity(const ChannelType Channel, PositionType Velocity) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	auto Result = StandaSyms::SA_CTL_SetProperty_i64(StandaHandle, Channel, SA_CTL_PKEY_MOVE_VELOCITY, Velocity);
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::MoveAbsolute(const ChannelType Channel, PositionType Position) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	auto Result = StandaSyms::SA_CTL_SetProperty_i32(StandaHandle, Channel, SA_CTL_PKEY_MOVE_MODE, SA_CTL_MOVE_MODE_CL_ABSOLUTE);
	//	CheckError(Result);

	//	Result = StandaSyms::SA_CTL_Move(StandaHandle, Channel, Position, 0);
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::MoveRelative(const ChannelType Channel, PositionType Position) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	auto Result = StandaSyms::SA_CTL_SetProperty_i32(StandaHandle, Channel, SA_CTL_PKEY_MOVE_MODE, SA_CTL_MOVE_MODE_CL_RELATIVE);
	//	CheckError(Result);

	//	Result = StandaSyms::SA_CTL_Move(StandaHandle, Channel, Position, 0);
	//	CheckError(Result);
	//}

	//void StandaHardwareAdapter::StopMotion(const ChannelType Channel) const
	//{
	//	auto lock = AcquireLock(HardwareOperationTimeout);

	//	if (!IsOpened())
	//		return;

	//	auto Result = StandaSyms::SA_CTL_Stop(StandaHandle, Channel, 0);
	//	CheckError(Result);
	//}
}