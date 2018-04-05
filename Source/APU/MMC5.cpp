/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "APU/MMC5.h"
#include "APU/Types.h"
#include "RegisterState.h"		// // //

// MMC5 external sound

CMMC5::CMMC5(CMixer &Mixer, std::uint8_t nInstance) :
	CSoundChip(Mixer, nInstance),		// // //
	m_Square1(Mixer, nInstance, sound_chip_t::MMC5, value_cast(mmc5_subindex_t::pulse1)),
	m_Square2(Mixer, nInstance, sound_chip_t::MMC5, value_cast(mmc5_subindex_t::pulse2)),
	m_iMulLow(0),
	m_iMulHigh(0)
{
	m_pRegisterLogger->AddRegisterRange(0x5000, 0x5007);		// // //
	m_pRegisterLogger->AddRegisterRange(0x5015, 0x5015);
}

sound_chip_t CMMC5::GetID() const {		// // //
	return sound_chip_t::MMC5;
}

void CMMC5::Reset()
{
	m_Square1.Reset();
	m_Square2.Reset();

	m_Square1.Write(0x01, 0x08);
	m_Square2.Write(0x01, 0x08);
}

void CMMC5::Write(uint16_t Address, uint8_t Value)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5) {
		m_iEXRAM[Address & 0x3FF] = Value;
		return;
	}

	switch (Address) {
		// Channel 1
		case 0x5000:
			m_Square1.Write(0, Value);
			break;
		case 0x5002:
			m_Square1.Write(2, Value);
			break;
		case 0x5003:
			m_Square1.Write(3, Value);
			break;
		// Channel 2
		case 0x5004:
			m_Square2.Write(0, Value);
			break;
		case 0x5006:
			m_Square2.Write(2, Value);
			break;
		case 0x5007:
			m_Square2.Write(3, Value);
			break;
		// Channel 3... (doesn't exist)
		// Control
		case 0x5015:
			m_Square1.WriteControl(Value & 1);
			m_Square2.WriteControl((Value >> 1) & 1);
			break;
		// Hardware multiplier
		case 0x5205:
			m_iMulLow = Value;
			break;
		case 0x5206:
			m_iMulHigh = Value;
			break;
	}
}

uint8_t CMMC5::Read(uint16_t Address, bool &Mapped)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5) {
		Mapped = true;
		return m_iEXRAM[Address & 0x3FF];
	}

	switch (Address) {
		case 0x5205:
			Mapped = true;
			return (m_iMulLow * m_iMulHigh) & 0xFF;
		case 0x5206:
			Mapped = true;
			return (m_iMulLow * m_iMulHigh) >> 8;
	}

	return 0;
}

void CMMC5::EndFrame()
{
	m_Square1.EndFrame();
	m_Square2.EndFrame();
}

void CMMC5::Process(uint32_t Time)
{
	m_Square1.Process(Time);
	m_Square2.Process(Time);
}

double CMMC5::GetFreq(int Channel) const		// // //
{
	switch (Channel) {
	case 0: return m_Square1.GetFrequency();
	case 1: return m_Square2.GetFrequency();
	}
	return 0.;
}

void CMMC5::LengthCounterUpdate()
{
	m_Square1.LengthCounterUpdate();
	m_Square2.LengthCounterUpdate();
}

void CMMC5::EnvelopeUpdate()
{
	m_Square1.EnvelopeUpdate();
	m_Square2.EnvelopeUpdate();
}

void CMMC5::ClockSequence()
{
	EnvelopeUpdate();		// // //
	LengthCounterUpdate();		// // //
}
