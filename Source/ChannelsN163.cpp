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

#include "ChannelsN163.h"
#include "APU/Types.h"		// // //
#include "APU/APUInterface.h"		// // //
#include "SeqInstrument.h"		// // //
#include "InstrumentN163.h"		// // // constants
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "SeqInstHandlerN163.h"		// // //
#include "SongState.h"		// // //
#include "FamiTrackerModule.h"		// // //
#include "Assertion.h"		// // //

const int N163_PITCH_SLIDE_SHIFT = 2;	// Increase amplitude of pitch slides

CChannelHandlerN163::CChannelHandlerN163(stChannelID ch) :		// / //
	CChannelHandlerInverted(ch, 0xFFFF, 0x0F),
	m_bDisableLoad(false),		// // //
	m_iWaveLen(4),		// // //
	m_iWaveCount(0),
	m_bResetPhase(false)
{
	m_iDutyPeriod = 0;
}

void CChannelHandlerN163::ResetChannel()
{
	CChannelHandler::ResetChannel();

	m_iWavePos = m_iWavePosOld = 0;		// // //
	m_iWaveLen = 4;
	m_bLoadWave = false;
}

void CChannelHandlerN163::ConfigureDocument(const CFamiTrackerModule &modfile) {		// // //
	CChannelHandler::ConfigureDocument(modfile);
	SetChannelCount(modfile.GetNamcoChannels());
}

bool CChannelHandlerN163::HandleEffect(ft0cc::doc::effect_command cmd)
{
	switch (cmd.fx) {
	case ft0cc::doc::effect_type::PORTA_DOWN:
		m_iPortaSpeed = cmd.param;
		if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
		m_iEffectParam = cmd.param;
		m_iEffect = m_bLinearPitch ? ft0cc::doc::effect_type::PORTA_DOWN : ft0cc::doc::effect_type::PORTA_UP;
		break;
	case ft0cc::doc::effect_type::PORTA_UP:
		m_iPortaSpeed = cmd.param;
		if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
		m_iEffectParam = cmd.param;
		m_iEffect = m_bLinearPitch ? ft0cc::doc::effect_type::PORTA_UP : ft0cc::doc::effect_type::PORTA_DOWN;
		break;
	case ft0cc::doc::effect_type::DUTY_CYCLE:
		// Duty effect controls wave
		m_iDefaultDuty = m_iDutyPeriod = cmd.param;
		m_bLoadWave = true;
		if (auto pHandler = dynamic_cast<CSeqInstHandlerN163 *>(m_pInstHandler.get()))
			pHandler->RequestWaveUpdate();
		break;
	case ft0cc::doc::effect_type::N163_WAVE_BUFFER:		// // //
		if (cmd.param == 0x7F) {
			m_iWavePos = m_iWavePosOld;
			m_bDisableLoad = false;
		}
		else {
			if (cmd.param + (m_iWaveLen >> 1) > 0x80 - 8 * m_iChannels) break;
			m_iWavePos = cmd.param << 1;
			m_bDisableLoad = true;
		}
		if (auto pHandler = dynamic_cast<CSeqInstHandlerN163 *>(m_pInstHandler.get()))
			pHandler->RequestWaveUpdate();
		break;
	default: return CChannelHandlerInverted::HandleEffect(cmd);
	}

	return true;
}

bool CChannelHandlerN163::HandleInstrument(bool Trigger, bool NewInstrument)
{
	if (!CChannelHandler::HandleInstrument(Trigger, NewInstrument))		// // //
		return false;

	if (!m_bLoadWave && NewInstrument)
		m_iDefaultDuty = m_iDutyPeriod = 0;

	if (!m_bDisableLoad) {
		m_iWavePos = /*pInstrument->GetAutoWavePos() ? GetChannelID().Subindex * 16 :*/ m_iWavePosOld;
	}

	return true;
}

void CChannelHandlerN163::HandleEmptyNote()
{
}

void CChannelHandlerN163::HandleCut()
{
	CutNote();
	m_iNote = -1;
	m_bRelease = false;
}

void CChannelHandlerN163::HandleRelease()
{
	if (!m_bRelease)
		ReleaseNote();
}

void CChannelHandlerN163::HandleNote(int MidiNote)
{
	// New note
	CChannelHandlerInverted::HandleNote(MidiNote);		// // //
//	m_bLoadWave = false;
//	m_bResetPhase = true;
}

bool CChannelHandlerN163::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler = std::make_unique<CSeqInstHandler>(this, 0x0F, Type == INST_S5B ? 0x40 : 0);
			return true;
		}
		break;
	case INST_N163:
		switch (m_iInstTypeCurrent) {
		case INST_N163: break;
		default:
			m_pInstHandler = std::make_unique<CSeqInstHandlerN163>(this, 0x0F, 0);
			return true;
		}
	}
	return false;
}

void CChannelHandlerN163::SetupSlide()		// // //
{
	CChannelHandler::SetupSlide();
	if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
}

void CChannelHandlerN163::RefreshChannel()
{
	int Channel = 7 - GetChannelID().Subindex;		// Channel #
	int WaveSize = 256 - (m_iWaveLen >> 2);
	int Frequency = CalculatePeriod();		// // //

	// Compensate for shorter waves
//	Frequency >>= 5 - int(log(double(m_iWaveLen)) / log(2.0));

	int Volume = CalculateVolume();
	int ChannelAddrBase = 0x40 + Channel * 8;

	if (!m_bGate)
		Volume = 0;

	// Update channel
	if (Channel + m_iChannels >= (int)MAX_CHANNELS_N163) {		// // //
		WriteData(ChannelAddrBase + 7, ((m_iChannels - 1) << 4) | Volume);
		if (!m_bGate)
			return;
		WriteData(ChannelAddrBase + 0, Frequency & 0xFF);
		WriteData(ChannelAddrBase + 2, (Frequency >> 8) & 0xFF);
		WriteData(ChannelAddrBase + 4, (WaveSize << 2) | ((Frequency >> 16) & 0x03));
		WriteData(ChannelAddrBase + 6, m_iWavePos);
	}

	if (m_bResetPhase) {
		m_bResetPhase = false;
		WriteData(ChannelAddrBase + 1, 0);
		WriteData(ChannelAddrBase + 3, 0);
		WriteData(ChannelAddrBase + 5, 0);
	}
}

void CChannelHandlerN163::SetWaveLength(int Length)		// // //
{
	Assert(Length >= 4 && Length <= CInstrumentN163::MAX_WAVE_SIZE && !(Length % 4));
	m_iWaveLen = Length;
}

void CChannelHandlerN163::SetWavePosition(int Pos)		// // //
{
	Assert(Pos >= 0 && Pos <= 0xFF);
	m_iWavePosOld = Pos;
}

void CChannelHandlerN163::SetWaveCount(int Count)		// // //
{
	Assert(Count > 0 && Count <= CInstrumentN163::MAX_WAVE_COUNT);
	m_iWaveCount = Count;
}

void CChannelHandlerN163::FillWaveRAM(array_view<const char> Wave)		// // //
{
	SetAddress(m_iWavePos >> 1, true);
	for (auto c : Wave)
		WriteData(c);
}

void CChannelHandlerN163::SetChannelCount(int Count)		// // //
{
	m_iChannels = Count;
}

int CChannelHandlerN163::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_2A03: case INST_VRC6: case INST_S5B:
		return -1;
	default:
		return Duty;
	}
}

void CChannelHandlerN163::ClearRegisters()
{
	int Channel = GetChannelID().Subindex;
	int ChannelAddrBase = 0x40 + Channel * 8;

	for (int i = 0; i < 8; ++i) {		// // //
		WriteReg(ChannelAddrBase + i, 0);
		WriteReg(ChannelAddrBase + i - 0x40, 0);
	}

	if (Channel == 7)		// // //
		WriteReg(ChannelAddrBase + 7, (m_iChannels - 1) << 4);

	m_bDisableLoad = false;		// // //
	m_iDutyPeriod = 0;
}

int CChannelHandlerN163::CalculatePeriod() const		// // //
{
	int Detune = GetVibrato() - GetFinePitch() - GetPitch();
	int Period = LimitPeriod(GetPeriod() + (Detune << 4));		// // //
	if (m_bLinearPitch && !m_iNoteLookupTable.empty()) {
		Period = LimitPeriod(GetPeriod() + Detune);		// // //
		int Note = Period >> LINEAR_PITCH_AMOUNT;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_iNoteLookupTable[Note + 1] - m_iNoteLookupTable[Note] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Period = m_iNoteLookupTable[Note] + Offset;
	}
	return LimitRawPeriod(Period) << N163_PITCH_SLIDE_SHIFT;
}

std::string CChannelHandlerN163::GetSlideEffectString() const		// // //
{
	if (m_iPortaSpeed) switch (m_iEffect) {
	case ft0cc::doc::effect_type::PORTA_UP:
		return MakeCommandString({ft0cc::doc::effect_type::PORTA_DOWN, static_cast<uint8_t>(m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT)});
	case ft0cc::doc::effect_type::PORTA_DOWN:
		return MakeCommandString({ft0cc::doc::effect_type::PORTA_UP, static_cast<uint8_t>(m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT)});
	case ft0cc::doc::effect_type::PORTAMENTO:
		return MakeCommandString({ft0cc::doc::effect_type::PORTAMENTO, static_cast<uint8_t>(m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT)});
	}
	return CChannelHandlerInverted::GetSlideEffectString();
}

std::string CChannelHandlerN163::GetCustomEffectString() const		// // //
{
	if (m_bDisableLoad)
		return MakeCommandString({ft0cc::doc::effect_type::N163_WAVE_BUFFER, static_cast<uint8_t>(m_iWavePos >> 1)});
	return std::string();
}

void CChannelHandlerN163::WriteReg(int Reg, int Value)
{
	m_pAPU->Write(0xF800, Reg);
	m_pAPU->Write(0x4800, Value);
}

void CChannelHandlerN163::SetAddress(char Addr, bool AutoInc)
{
	m_pAPU->Write(0xF800, (AutoInc ? 0x80 : 0) | Addr);
}

void CChannelHandlerN163::WriteData(char Data)
{
	m_pAPU->Write(0x4800, Data);
}

void CChannelHandlerN163::WriteData(int Addr, char Data)
{
	SetAddress(Addr, false);
	WriteData(Data);
}
