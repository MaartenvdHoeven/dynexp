// This file is part of DynExp.

/**
 * @file HardwareAdapterStanda8SMC5.h
 * @brief Implementation of a hardware adapter to control Standa 8SMC5
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/Stage.h"

namespace DynExpHardware::StandaSyms
{
	#include "../include/Standa/ximc.h"
}

namespace DynExpHardware
{
	class StandaHardwareAdapter;

	class StandaException : public Util::Exception
	{
	public:
		StandaException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Util::ErrorType::Error, ErrorCode, Location)
		{}
	};

	class StandaHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		StandaHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}
		virtual ~StandaHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "StandaHardwareAdapterParams"; }

		Param<TextList> DeviceName = { *this, {}, "DeviceName", "Device name", "Name of the Standa controller to connect with" };


	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<StandaHardwareAdapterParams>) {}
	};

	class StandaHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = StandaHardwareAdapter;
		using ParamsType = StandaHardwareAdapterParams;

		StandaHardwareAdapterConfigurator() = default;
		virtual ~StandaHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<StandaHardwareAdapterConfigurator>(ID, Core); }
	};

	class StandaHardwareAdapter : public DynExp::HardwareAdapterBase
	{
	public:
		using ParamsType = StandaHardwareAdapterParams;
		using ConfigType = StandaHardwareAdapterConfigurator;

		using ChannelType = int8_t;
		using PositionType = int64_t;
		using DirectionType = DynExpInstr::PositionerStage::DirectionType;

		constexpr static auto Name() noexcept { return "Standa"; }
		constexpr static auto Category() noexcept { return "Positioners"; }
		static std::vector<std::string> Enumerate();

		StandaHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~StandaHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool IsOpened() const noexcept { return DeviceHandle != device_undefined; }

		PositionType GetCurrentPosition(const ChannelType Channel) const;
		PositionType GetTargetPosition(const ChannelType Channel) const;

		PositionType GetVelocity(const ChannelType Channel) const;

		void SetVelocity(const ChannelType Channel, PositionType Velocity) const;

		void MoveAbsolute(const ChannelType Channel, PositionType Position) const;
		void MoveRelative(const ChannelType Channel, PositionType Position) const;

		void StopMotion(const ChannelType Channel) const;

	private:
		void Init();

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<StandaHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		void CheckError(const DynExpHardware::StandaSyms::result_t Result, const std::source_location Location = std::source_location::current()) const;

		void OpenUnsafe();
		void CloseUnsafe();

		std::string DeviceName;

		DynExpHardware::StandaSyms::device_t DeviceHandle = device_undefined;
	};
}