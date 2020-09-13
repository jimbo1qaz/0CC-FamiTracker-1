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


#pragma once

// // // 050B

#include <string>
#include <string_view>
#include <optional>
#include <memory>

class CSequence;

/*!
	\brief A realization of the CSequenceStringConversion class from FamiTracker 0.5.0 beta, which
	handles conversion between sequence values and MML string terms.
*/
class CSeqConversionBase
{
public:
	/*!	\brief Converts a sequence value to a string.
		\param Value Input value from the sequence.
		\return A unique string representation of the sequence value. */
	virtual std::string ToString(char Value) const = 0; // TODO: maybe use a different class for this

	/*!	\brief Converts an MML string term to a sequence value.
		\details A string term may represent any number of sequence values, including zero.
		\param String A single MML string term.
		\return True if the string is a valid representation. */
	virtual bool ToValue(std::string_view sv) = 0;
	/*!	\brief Checks the availability of the current string conversion.
		\details For each valid MML string term, **as well as before and after converting an MML
		string**, a new value is expected as long as this method returns true.
		\return True if a new value is ready for use. */
	virtual bool IsReady() const = 0;
	/*!	\brief Extracts a value from a string conversion.
		\returns The sequence value. */
	virtual char GetValue() = 0;

	/*!	\brief Called before converting an MML string. */
	virtual void OnStart() { }
	/*!	\brief Called after converting an MML string. */
	virtual void OnFinish() { }

	/*!
	hertzdevil was... misguided... in so many ways
	he went down his own path
	a path of refactoring without tests
	a lonely road of template metaprogramming
	rejecting contributors with bugs or fixes, not acknowledging their solutions
	i saw an image of unapproachability.

	negligence in some aspects
	no CI to make sure the code built in both debug and release
	it didn't, for dozens of commits on end... a tarpit for would-be `git bisect`ors
	switching to UNICODE and gibberish appearing in modules

	i don't know if he was incompetent
	(shitty register view code, possibly he copied what jsr did,
	but the font size differs from 0.5.0 beta)...

	trying to look smart and writing broken code
	(std::unique_ptr<> {m_pRecentFileList} = std::make_unique<>(...))
	he saw that unique_ptr was Good. he used it incorrectly.
	the code didn't work and he didn't check if it worked.

	or brilliant (he did some 0.5.0 beta reverse engineering streams,
	probably better at binary reversing than me)

	i think he was a perfectionist in some ways... obsessive refactoring,
	trying to reshape the code to his ends, mud escaping between his fingers

	then he left... and the code rotted, and visual studio shuffled their headers,
	and suddenly the code stopped compiling

	happened to doctest too, suddenly their stream-output code stopped working
	and you had to add extra #includes

	oops hertz forgot a virtual destructor
	*/
	virtual ~CSeqConversionBase() = default;
};

class CSeqConversionDefault : public CSeqConversionBase
{
public:
	CSeqConversionDefault() { }
	CSeqConversionDefault(int Min, int Max) : m_iMinValue(Min), m_iMaxValue(Max) { }

public:
	std::string ToString(char Value) const override;
	bool ToValue(std::string_view sv) override;
	bool IsReady() const override;
	char GetValue() override;
	void OnStart() override;
	void OnFinish() override;

protected:
	std::optional<int> GetNextInteger(std::string_view &sv, bool Signed = false) const;
	virtual std::optional<int> GetNextTerm(std::string_view &sv);

protected:
	const int m_iMinValue = INT32_MIN;
	const int m_iMaxValue = INT32_MAX;

private:
	bool m_bReady = false;
	bool m_bHex;

	int m_iCurrentValue, m_iCounter, m_iTargetValue;
	int m_iValueDiv, m_iValueMod, m_iValueInc;
	int m_iRepeat, m_iRepeatCounter;
};

class CSeqConversion5B : public CSeqConversionDefault
{
public:
	CSeqConversion5B() : CSeqConversionDefault(0, 0x1F) { }
	std::string ToString(char Value) const override;
	bool ToValue(std::string_view sv) override;
	char GetValue() override;
protected:
	std::optional<int> GetNextTerm(std::string_view &sv) override;
private:
	char m_iEnableFlags;
};

class CSeqConversionArpScheme : public CSeqConversionDefault		// // //
{
public:
	CSeqConversionArpScheme(int Min) : CSeqConversionDefault(Min, Min + 63) { }
	std::string ToString(char Value) const override;
	bool ToValue(std::string_view sv) override;
	char GetValue() override;
protected:
	std::optional<int> GetNextTerm(std::string_view &sv) override;
private:
	char m_iArpSchemeFlag;
};

class CSeqConversionArpFixed : public CSeqConversionDefault		// // //
{
public:
	CSeqConversionArpFixed() : CSeqConversionDefault(0, 95) { }
	std::string ToString(char Value) const override;
protected:
	std::optional<int> GetNextTerm(std::string_view &sv) override;
};

/*!
	\brief A class which uses a conversion object to translate between string representations and
	instrument sequence values.
*/
class CSequenceParser
{
public:
	/*!	\brief Changes the instrument sequence used by the parser.
		\param pSeq Pointer to the new sequence. */
	void SetSequence(std::shared_ptr<CSequence> pSeq);
	/*!	\brief Changes the conversion algorithm.
		\details The sequence parser owns the conversion object and handles its deletion.
		\param pConv Pointer to the new sequence conversion object. */
	void SetConversion(std::unique_ptr<CSeqConversionBase> pConv);
	/*!	\brief Updates the current instrument sequence from an MML string.
		\param String the input string. */
	void ParseSequence(std::string_view sv);
	/*!	\brief Obtains a string representation of the current instrument sequence.
		\return An MML string which represents the sequence. */
	std::string PrintSequence() const;

private:
	unsigned int m_iPushedCount = 0;
	std::shared_ptr<CSequence> m_pSequence;
	std::unique_ptr<CSeqConversionBase> m_pConversion;
};
