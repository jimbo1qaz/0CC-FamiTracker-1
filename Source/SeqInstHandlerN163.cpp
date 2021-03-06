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

#include "SeqInstHandlerN163.h"
#include "ChannelHandlerInterface.h"

/*
 * Class CSeqInstHandlerN163
 */

CSeqInstHandlerN163::CSeqInstHandlerN163(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
	CSeqInstHandler(pInterface, Vol, Duty),
	m_pBufferCurrent(m_cBuffer),
	m_pBufferPrevious(m_cBuffer + CInstrumentN163::MAX_WAVE_SIZE),
	m_bForceUpdate(false)
{
}

void CSeqInstHandlerN163::LoadInstrument(std::shared_ptr<CInstrument> pInst)		// // //
{
	CSeqInstHandler::LoadInstrument(pInst);
	CChannelHandlerInterfaceN163 *pInterface = dynamic_cast<CChannelHandlerInterfaceN163*>(m_pInterface);
	if (pInterface == nullptr) return;
	auto pN163Inst = std::dynamic_pointer_cast<const CInstrumentN163>(m_pInstrument);
	if (pN163Inst == nullptr) return;
	pInterface->SetWaveLength(pN163Inst->GetWaveSize());
	pInterface->SetWavePosition(pN163Inst->GetWavePos());
	pInterface->SetWaveCount(pN163Inst->GetWaveCount());
	RequestWaveUpdate();
}

void CSeqInstHandlerN163::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();
	RequestWaveUpdate();
}

void CSeqInstHandlerN163::UpdateInstrument()
{
	CSeqInstHandler::UpdateInstrument();

	if (dynamic_cast<CChannelHandlerInterfaceN163*>(m_pInterface))
		if (auto pN163Inst = std::dynamic_pointer_cast<const CInstrumentN163>(m_pInstrument))
			UpdateWave(*pN163Inst);
	m_bForceUpdate = false;
}

void CSeqInstHandlerN163::RequestWaveUpdate()
{
	m_bForceUpdate = true;
}

void CSeqInstHandlerN163::UpdateWave(const CInstrumentN163 &Inst)
{
	char *Temp = m_pBufferPrevious;
	m_pBufferPrevious = m_pBufferCurrent;
	m_pBufferCurrent = Temp;

	// raw position and count
	// int Duty = m_pInterface->GetDutyPeriod();
	// if (Duty < 0) return;
	int Index = m_pInterface->GetDutyPeriod();
	if (Index >= Inst.GetWaveCount())
		Index = Inst.GetWaveCount() - 1;
	const unsigned Count = Inst.GetWaveSize() >> 1;
	for (unsigned i = 0; i < Count; ++i)
		m_pBufferCurrent[i] = Inst.GetSample(Index, 2 * i) | (Inst.GetSample(Index, 2 * i + 1) << 4);

	if (auto pInterface = dynamic_cast<CChannelHandlerInterfaceN163 *>(m_pInterface))
		if (array_view<const char> {m_pBufferCurrent, Count} != array_view<const char> {m_pBufferPrevious, Count} || m_bForceUpdate)
			pInterface->FillWaveRAM({m_pBufferCurrent, Inst.GetWaveSize() >> 1});
}
