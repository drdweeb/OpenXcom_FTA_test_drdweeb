#pragma once
/*
 * Copyright 2010-2022 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../Engine/State.h"
#include <vector>
#include <set>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class ComboBox;
class Base;
class Soldier;
class RuleResearch;
class Production;

/**
* Allocate Engineers screen that lets the player
* pick the personnel to assign to the facility construction.
*/
class FacilityAllocateEngineersState : public State
{
private:
	TextButton* _btnOk, *_btnInfo;
	Window* _window;
	Text* _txtTitle, * _txtName, *_txtTime, * _txtAssignment, *_txtStat;
	TextList* _lstEngineers;
	Base* _base;
	Production* _production;
	std::set<Soldier*> _engineers;
	Uint8 _otherCraftColor;
	std::vector<Soldier*> _origSoldierOrder;
	std::vector<int> _engineerNumbers;
	/// initializes the display list based on the project soldier's list and the position to display
	void initList(size_t scrl);
	std::string getReqTime();

public:
	/// Creates the Craft Soldiers state.
	FacilityAllocateEngineersState(Base* base, Production* production);
	/// Cleans up the Craft Soldiers state.
	~FacilityAllocateEngineersState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action* action);
	/// Handler for clicking the INFO button.
	void btnInfoClick(Action* action);
	/// Updates the soldiers list.
	void init() override;
	/// Handler for clicking the Soldiers list.
	void lstEngineersClick(Action* action);
};

}
