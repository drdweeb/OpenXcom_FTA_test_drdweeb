/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include "MonthlyCostsState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Unicode.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleSoldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Monthly Costs screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
MonthlyCostsState::MonthlyCostsState(Base *base) : _base(base)
{
	bool isFta = _game->getMod()->isFTAGame();
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(300, 20, 10, 170);
	_txtTitle = new Text(310, 17, 5, 12);
	_txtCost = new Text(80, 9, 115, 32);
	_txtQuantity = new Text(55, 9, 195, 32);
	_txtTotal = new Text(60, 9, 249, 32);
	_txtRental = new Text(150, 9, 10, 40);
	_txtSalaries = new Text(150, 9, 10, 80);
	if (isFta)
	{
		_txtMaintenance = new Text(150, 9, 10, 150);
	}
	else //vanilla
	{
		_txtIncome = new Text(150, 9, 10, 146);
		_txtMaintenance = new Text(150, 9, 10, 154);
	}
	
	_lstCrafts = new TextList(288, 32, 10, 48);
	_lstSalaries = new TextList(288, 40, 10, 88);
	_lstMaintenance = new TextList(300, 9, 10, 128);
	_lstTotal = new TextList(100, 9, 205, 150);

	// Set palette
	setInterface("costsInfo");

	add(_window, "window", "costsInfo");
	add(_btnOk, "button", "costsInfo");
	add(_txtTitle, "text1", "costsInfo");
	add(_txtCost, "text1", "costsInfo");
	add(_txtQuantity, "text1", "costsInfo");
	add(_txtTotal, "text1", "costsInfo");
	add(_txtRental, "text1", "costsInfo");
	add(_lstCrafts, "list", "costsInfo");
	add(_txtSalaries, "text1", "costsInfo");
	add(_lstSalaries, "list", "costsInfo");
	add(_lstMaintenance, "text1", "costsInfo");
	if (!isFta)
	{
		add(_txtIncome, "list", "costsInfo");
	}
	add(_txtMaintenance, "list", "costsInfo");
	add(_lstTotal, "text2", "costsInfo");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "costsInfo");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&MonthlyCostsState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MonthlyCostsState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&MonthlyCostsState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MONTHLY_COSTS"));

	_txtCost->setText(tr("STR_COST_PER_UNIT"));

	_txtQuantity->setText(tr("STR_QUANTITY"));

	_txtTotal->setText(tr("STR_TOTAL"));

	_txtRental->setText(tr("STR_CRAFT_RENTAL"));

	_txtSalaries->setText(tr("STR_SALARIES"));

	if (!isFta)
	{
		std::ostringstream ss;
		ss << tr("STR_INCOME") << "=" << Unicode::formatFunding(_game->getSavedGame()->getCountryFunding());
		_txtIncome->setText(ss.str());
	}

	std::ostringstream ss2;
	ss2 << tr("STR_MAINTENANCE") << "=" << Unicode::formatFunding(_game->getSavedGame()->getBaseMaintenance());
	_txtMaintenance->setText(ss2.str());

	_lstCrafts->setColumns(4, 125, 70, 44, 50);
	_lstCrafts->setDot(true);

	const std::vector<std::string> &crafts = _game->getMod()->getCraftsList();
	for (std::vector<std::string>::const_iterator i = crafts.begin(); i != crafts.end(); ++i)
	{
		RuleCraft *craft = _game->getMod()->getCraft(*i);
		if (craft->getRentCost() != 0 && (_game->getSavedGame()->isResearched(craft->getRequirements()) || _game->getMod()->isFTAGame()))
		{
			auto count = _base->getCraftCount(craft);
			if (count > 0 || craft->forceShowInMonthlyCosts())
			{
				std::ostringstream ss3;
				ss3 << count;
				_lstCrafts->addRow(4, tr(*i).c_str(), Unicode::formatFunding(craft->getRentCost()).c_str(), ss3.str().c_str(), Unicode::formatFunding(count * craft->getRentCost()).c_str());
			}
		}
	}

	_lstSalaries->setColumns(4, 125, 70, 44, 50);
	_lstSalaries->setDot(true);

	const std::vector<std::string> &soldiers = _game->getMod()->getSoldiersList();

	bool dynamicSalaries = false;
	for (std::vector<std::string>::const_iterator i = soldiers.begin(); i != soldiers.end(); ++i)
	{
		if (_game->getMod()->getSoldier(*i)->isSalaryDynamic())
		{
			dynamicSalaries = true;
			break;
		}
	}

	if (!dynamicSalaries)
	{
		// vanilla
		for (std::vector<std::string>::const_iterator i = soldiers.begin(); i != soldiers.end(); ++i)
		{
			const RuleSoldier *soldier = _game->getMod()->getSoldier(*i);
			if (soldier->getSalaryCost(0) != 0 && _game->getSavedGame()->isResearched(soldier->getRequirements()))
			{
				std::pair<int, int> info = _base->getSoldierCountAndSalary(*i);
				std::ostringstream ss4;
				ss4 << info.first;
				std::string name = (*i);
				if (soldiers.size() == 1)
				{
					name = "STR_SOLDIERS";
				}
				std::string costPerUnit = Unicode::formatFunding(soldier->getSalaryCost(0)); // 0 = default rookie salary
				if (info.first > 0 || soldiers.size() == 1)
				{
					_lstSalaries->addRow(4, tr(name).c_str(), costPerUnit.c_str(), ss4.str().c_str(), Unicode::formatFunding(info.second).c_str());
				}
			}
		}
	}
	else
	{
		// one or more soldier types with *different* salaries per rank
		std::ostringstream ss4;
		int count = 0;
		int salary = 0;
		for (std::vector<std::string>::const_iterator i = soldiers.begin(); i != soldiers.end(); ++i)
		{
			std::pair<int, int> info = _base->getSoldierCountAndSalary(*i);
			count += info.first;
			salary += info.second;
		}
		ss4 << count;
		_lstSalaries->addRow(4, tr("STR_SOLDIERS").c_str(), "", ss4.str().c_str(), Unicode::formatFunding(salary).c_str());
	}
	std::ostringstream ss5;
	ss5 << _base->getTotalEngineers();
	_lstSalaries->addRow(4, tr("STR_ENGINEERS").c_str(), Unicode::formatFunding(_game->getMod()->getEngineerCost()).c_str(), ss5.str().c_str(), Unicode::formatFunding(_base->getTotalEngineers() * _game->getMod()->getEngineerCost()).c_str());
	std::ostringstream ss6;
	ss6 << _base->getTotalScientists();
	_lstSalaries->addRow(4, tr("STR_SCIENTISTS").c_str(), Unicode::formatFunding(_game->getMod()->getScientistCost()).c_str(), ss6.str().c_str(), Unicode::formatFunding(_base->getTotalScientists() * _game->getMod()->getScientistCost()).c_str());
	std::ostringstream ss6b;
	int staffCount, inventoryCount;
	int totalOtherCost = _base->getTotalOtherStaffAndInventoryCost(staffCount, inventoryCount);
	ss6b << staffCount << "/" << inventoryCount;
	_lstSalaries->addRow(4, tr("STR_OTHER_EMPLOYEES").c_str(), "", ss6b.str().c_str(), Unicode::formatFunding(totalOtherCost).c_str());

	_lstMaintenance->setColumns(2, 239, 60);
	_lstMaintenance->setDot(true);
	std::ostringstream ss7;
	ss7 << Unicode::TOK_COLOR_FLIP << Unicode::formatFunding(_base->getFacilityMaintenance());
	_lstMaintenance->addRow(2, tr("STR_BASE_MAINTENANCE").c_str(), ss7.str().c_str());

	_lstTotal->setColumns(2, 44, 55);
	_lstTotal->setDot(true);
	_lstTotal->addRow(2, tr("STR_TOTAL").c_str(), Unicode::formatFunding(_base->getMonthlyMaintenace()).c_str());
}

/**
 *
 */
MonthlyCostsState::~MonthlyCostsState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MonthlyCostsState::btnOkClick(Action *)
{
	_game->popState();
}

}
