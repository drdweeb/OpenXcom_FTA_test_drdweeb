/*
 * Copyright 2010-2021 OpenXcom Developers.
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
#include "DiplomacyPurchaseState.h"
#include <sstream>
#include <climits>
#include <iomanip>
#include <algorithm>
#include <locale>
#include "../fmath.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Unicode.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/Base.h"
#include "../Engine/Action.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/Armor.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Savegame/DiplomacyFaction.h"
#include "../Savegame/FactionalContainer.h"
#include "../Mod/RuleDiplomacyFaction.h"
#include "../Engine/Logger.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the Purchase/Hire screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
DiplomacyPurchaseState::DiplomacyPurchaseState(Base *base, DiplomacyFaction* faction) : _base(base), _faction(faction),
		_sel(0), _total(0), _pQty(0), _cQty(0), _iQty(0.0), _ammoColor(0)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 13);
	_btnOk = new TextButton(148, 16, 8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtFunds = new Text(150, 9, 10, 24);
	_txtPurchases = new Text(150, 9, 160, 24);
	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtCost = new Text(85, 9, 152, 44);
	_txtQuantity = new Text(51, 9, 256, 44);
	_cbxCategory = new ComboBox(this, 120, 16, 10, 36);
	_lstItems = new TextList(288, 120, 8, 54);

	// Set palette
	setInterface("buyMenu");

	_ammoColor = _game->getMod()->getInterface("buyMenu")->getElement("ammoColor")->color;

	add(_window, "window", "buyMenu");
	add(_btnQuickSearch, "button", "buyMenu");
	add(_btnOk, "button", "buyMenu");
	add(_btnCancel, "button", "buyMenu");
	add(_txtTitle, "text", "buyMenu");
	add(_txtFunds, "text", "buyMenu");
	add(_txtPurchases, "text", "buyMenu");
	add(_txtSpaceUsed, "text", "buyMenu");
	add(_txtCost, "text", "buyMenu");
	add(_txtQuantity, "text", "buyMenu");
	add(_lstItems, "list", "buyMenu");
	add(_cbxCategory, "text", "buyMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "buyMenu");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&DiplomacyPurchaseState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&DiplomacyPurchaseState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&DiplomacyPurchaseState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&DiplomacyPurchaseState::btnCancelClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PURCHASE_HIRE_PERSONNEL"));

	_txtFunds->setText(tr("STR_CURRENT_FUNDS").arg(Unicode::formatFunding(_game->getSavedGame()->getFunds())));

	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES").arg(Unicode::formatFunding(_total)));

	_txtSpaceUsed->setVisible(true); // (Options::storageLimitsEnforced);
	std::ostringstream ss;
	ss << _base->getUsedStores() << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss.str()));

	_txtCost->setText(tr("STR_COST_PER_UNIT_UC"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_lstItems->setArrowColumn(211, ARROW_VERTICAL);
	_lstItems->setColumns(5, 132, 51, 53, 21, 30);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onLeftArrowPress((ActionHandler)&DiplomacyPurchaseState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)&DiplomacyPurchaseState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)&DiplomacyPurchaseState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)&DiplomacyPurchaseState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)&DiplomacyPurchaseState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)&DiplomacyPurchaseState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)&DiplomacyPurchaseState::lstItemsMousePress);

	_cats.push_back("STR_ALL_ITEMS");
	_cats.push_back("STR_FILTER_HIDDEN");

	auto providedBaseFunc = _base->getProvidedBaseFunc({});
	const std::vector<std::string> &soldiers = _game->getMod()->getSoldiersList();
	for (std::vector<std::string>::const_iterator i = soldiers.begin(); i != soldiers.end(); ++i)
	{
		const RuleSoldier *rule = _game->getMod()->getSoldier(*i);
		auto purchaseBaseFunc = rule->getRequiresBuyBaseFunc();
		int stock = getFactionItemStock(rule->getType());
		if (rule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(rule->getRequirements())
			&& (~providedBaseFunc & purchaseBaseFunc).none()
			&& stock > 0)
		{
			TransferRow row = { TRANSFER_SOLDIER, rule, tr(rule->getType()), getCostAdjustment(rule->getBuyCost()), _base->getSoldierCountAndSalary(rule->getType()).first, 0, 0, stock, -4, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	{
		int stock = getFactionItemStock("STR_SCIENTIST");
		if (stock > 0 && (_game->getMod()->getHireScientistsUnlockResearch().empty() || _game->getSavedGame()->isResearched(_game->getMod()->getHireScientistsUnlockResearch(), true)))
		{
			TransferRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"), getCostAdjustment(_game->getMod()->getHireScientistCost()), _base->getTotalScientists(), 0, 0, stock, -3, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	{
		int stock = getFactionItemStock("STR_ENGINEER");
		if (stock > 0 && (_game->getMod()->getHireEngineersUnlockResearch().empty() || _game->getSavedGame()->isResearched(_game->getMod()->getHireEngineersUnlockResearch(), true)))
		{
			TransferRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"), getCostAdjustment(_game->getMod()->getHireEngineerCost()), _base->getTotalEngineers(), 0, 0, stock, -2, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}

	}
	const std::vector<std::string> &crafts = _game->getMod()->getCraftsList();
	for (std::vector<std::string>::const_iterator i = crafts.begin(); i != crafts.end(); ++i)
	{
		RuleCraft *rule = _game->getMod()->getCraft(*i);
		auto purchaseBaseFunc = rule->getRequiresBuyBaseFunc();
		int stock = getFactionItemStock(rule->getType());
		if (rule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(rule->getRequirements())
			&& (~providedBaseFunc & purchaseBaseFunc).none()
			&& stock > 0)
		{
			TransferRow row = { TRANSFER_CRAFT, rule, tr(rule->getType()), getCostAdjustment(rule->getBuyCost()), _base->getCraftCount(rule), 0, 0, stock, -1, 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		RuleItem *rule = _game->getMod()->getItem(*i);
		auto purchaseBaseFunc = rule->getRequiresBuyBaseFunc();
		int stock = getFactionItemStock(rule->getName());
		if (rule->getBuyCost() != 0
			&& _game->getSavedGame()->isResearched(rule->getRequirements())
			&& _game->getSavedGame()->isResearched(rule->getBuyRequirements())
			&& (~providedBaseFunc & purchaseBaseFunc).none()
			&& stock > 0)
		{
			TransferRow row = { TRANSFER_ITEM, rule, tr(rule->getType()), getCostAdjustment(rule->getBuyCost()), _base->getStorageItems()->getItem(rule), 0, 0, stock, rule->getListOrder(), 0, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	_vanillaCategories = _cats.size();
	if (_game->getMod()->getDisplayCustomCategories() > 0)
	{
		bool hasUnassigned = false;

		// first find all relevant item categories
		std::vector<std::string> tempCats;
		for (std::vector<TransferRow>::iterator i = _items.begin(); i != _items.end(); ++i)
		{
			if ((*i).type == TRANSFER_ITEM)
			{
				RuleItem *rule = (RuleItem*)((*i).rule);
				if (rule->getCategories().empty())
				{
					hasUnassigned = true;
				}
				for (std::vector<std::string>::const_iterator j = rule->getCategories().begin(); j != rule->getCategories().end(); ++j)
				{
					if (std::find(tempCats.begin(), tempCats.end(), (*j)) == tempCats.end())
					{
						tempCats.push_back((*j));
					}
				}
			}
		}
		// then use them nicely in order
		if (_game->getMod()->getDisplayCustomCategories() == 1)
		{
			_cats.clear();
			_cats.push_back("STR_ALL_ITEMS");
			_cats.push_back("STR_FILTER_HIDDEN");
			_vanillaCategories = _cats.size();
		}
		const std::vector<std::string> &categories = _game->getMod()->getItemCategoriesList();
		for (std::vector<std::string>::const_iterator k = categories.begin(); k != categories.end(); ++k)
		{
			if (std::find(tempCats.begin(), tempCats.end(), (*k)) != tempCats.end())
			{
				_cats.push_back((*k));
			}
		}
		if (hasUnassigned)
		{
			_cats.push_back("STR_UNASSIGNED");
		}
	}

	_cbxCategory->setOptions(_cats, true);
	_cbxCategory->onChange((ActionHandler)&DiplomacyPurchaseState::cbxCategoryChange);

	_btnQuickSearch->setText(""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&DiplomacyPurchaseState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&DiplomacyPurchaseState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	updateList();

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&DiplomacyPurchaseState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&DiplomacyPurchaseState::decrease);
}

/**
 *
 */
DiplomacyPurchaseState::~DiplomacyPurchaseState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
 * Runs the arrow timers.
 */
void DiplomacyPurchaseState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Calculates price adjustment based on faction's stats and other factors.
 * @param baseCost price for row from rules.
 * @returns corrected value.
 */
int DiplomacyPurchaseState::getCostAdjustment(int baseCost)
{
	int priceFactor = _faction->getRules()->getBuyPriceFactor();
	int repFactor = _faction->getRules()->getRepPriceFactor();
	int normalizedRep = _faction->getReputationLevel() - 3;
	int64_t result = baseCost;
	if (priceFactor != 0 && repFactor != 0 && normalizedRep != 0)
	{
		result *= priceFactor / 100;
		result *= normalizedRep / 100;
	}

	return static_cast<int>(result);
}

/**
 * Determines the category a row item belongs in.
 * @param sel Selected row.
 * @returns Item category.
 */
std::string DiplomacyPurchaseState::getCategory(int sel) const
{
	RuleItem* rule = 0;
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		return "STR_PERSONNEL";
	case TRANSFER_CRAFT:
		return "STR_CRAFT_ARMAMENT";
	case TRANSFER_ITEM:
		rule = (RuleItem*)_items[sel].rule;
		if (rule->getBattleType() == BT_CORPSE || rule->isAlien())
		{
			if (rule->getVehicleUnit())
				return "STR_PERSONNEL"; // OXCE: critters fighting for us
			if (rule->isAlien())
				return "STR_PRISONERS"; // OXCE: live aliens
			return "STR_ALIENS";
		}
		if (rule->getBattleType() == BT_NONE)
		{
			if (_game->getMod()->isCraftWeaponStorageItem(rule))
			{
				return "STR_CRAFT_ARMAMENT";
			}
			if (_game->getMod()->isArmorStorageItem(rule))
			{
				return "STR_ARMORS"; // OXCE: armors
			}
			return "STR_COMPONENTS";
		}
		return "STR_EQUIPMENT";
	}
	return "STR_ALL_ITEMS";
}

/**
 * Determines if a row item belongs to a given category.
 * @param sel Selected row.
 * @param cat Category.
 * @returns True if row item belongs to given category, otherwise False.
 */
bool DiplomacyPurchaseState::belongsToCategory(int sel, const std::string &cat) const
{
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
	case TRANSFER_CRAFT:
		return false;
	case TRANSFER_ITEM:
		RuleItem *rule = (RuleItem*)_items[sel].rule;
		return rule->belongsToCategory(cat);
	}
	return false;
}

/**
 * Determines if a row item is supposed to be hidden
 * @param sel Selected row.
 * @param cat Category.
 * @returns True if row item is hidden
 */
bool DiplomacyPurchaseState::isHidden(int sel) const
{
	std::string itemName;
	bool isCraft = false;

	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		return false;
	case TRANSFER_CRAFT:
		isCraft = true; // fall-through
	case TRANSFER_ITEM:
		if (isCraft)
		{
			RuleCraft *rule = (RuleCraft*)_items[sel].rule;
			if (rule != 0)
			{
				itemName = rule->getType();
			}
		}
		else
		{
			RuleItem *rule = (RuleItem*)_items[sel].rule;
			if (rule != 0)
			{
				itemName = rule->getType();
			}
		}
		if (!itemName.empty())
		{
			std::map<std::string, bool> hiddenMap = _game->getSavedGame()->getHiddenPurchaseItems();
			std::map<std::string, bool>::const_iterator iter = hiddenMap.find(itemName);
			if (iter != hiddenMap.end())
			{
				return iter->second;
			}
			else
			{
				// not found = not hidden
				return false;
			}
		}
	}

	return false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void DiplomacyPurchaseState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText("");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
* Quick search.
* @param action Pointer to an action.
*/
void DiplomacyPurchaseState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
 * Filters the current list of items.
 */
void DiplomacyPurchaseState::updateList()
{
	std::string searchString = _btnQuickSearch->getText();
	Unicode::upperCase(searchString);

	_lstItems->clearList();
	_rows.clear();

	size_t selCategory = _cbxCategory->getSelected();
	const std::string selectedCategory = _cats[selCategory];
	bool categoryFilterEnabled = (selectedCategory != "STR_ALL_ITEMS");
	bool categoryUnassigned = (selectedCategory == "STR_UNASSIGNED");
	bool categoryHidden = (selectedCategory == "STR_FILTER_HIDDEN");

	for (size_t i = 0; i < _items.size(); ++i)
	{
		// filter
		bool hidden = isHidden(i);
		if (categoryHidden)
		{
			if (!hidden)
			{
				continue;
			}
		}
		else if (hidden)
		{
			continue;
		}
		else if (selCategory >= _vanillaCategories)
		{
			if (categoryUnassigned && _items[i].type == TRANSFER_ITEM)
			{
				RuleItem* rule = (RuleItem*)_items[i].rule;
				if (!rule->getCategories().empty())
				{
					continue;
				}
			}
			else if (categoryFilterEnabled && !belongsToCategory(i, selectedCategory))
			{
				continue;
			}
		}
		else
		{
			if (categoryFilterEnabled && selectedCategory != getCategory(i))
			{
				continue;
			}
		}

		// quick search
		if (!searchString.empty())
		{
			std::string projectName = _items[i].name;
			Unicode::upperCase(projectName);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		std::string name = _items[i].name;
		bool ammo = false;
		if (_items[i].type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)_items[i].rule;
			ammo = (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0));
			if (ammo)
			{
				name.insert(0, "  ");
			}
		}
		std::ostringstream ssQty, ssAmount, ssStock;
		ssQty << _items[i].qtySrc;
		ssAmount << _items[i].amount;
		ssStock << "/" << _items[i].stock;
		_lstItems->addRow(5, name.c_str(), Unicode::formatFunding(_items[i].cost).c_str(), ssQty.str().c_str(), ssAmount.str().c_str(), ssStock.str().c_str());
		_rows.push_back(i);
		if (_items[i].amount > 0)
		{
			_lstItems->setRowColor(_rows.size() - 1, _lstItems->getSecondaryColor());
		}
		else if (ammo)
		{
			_lstItems->setRowColor(_rows.size() - 1, _ammoColor);
		}
	}
}

/**
 * Purchases the selected items.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::btnOkClick(Action *)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _total);
	_faction->setFunds(_faction->getFunds() + _total);
	for (std::vector<TransferRow>::const_iterator i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->amount > 0)
		{
			Transfer *t = 0;
			switch (i->type)
			{
			case TRANSFER_SOLDIER:
				for (int s = 0; s < i->amount; s++)
				{
					const RuleSoldier *rule = (RuleSoldier*)i->rule;
					int time = rule->getTransferTime();
					if (time == 0)
						time = _game->getMod()->getPersonnelTime();
					t = new Transfer(time);
					int nationality = _game->getSavedGame()->selectSoldierNationalityByLocation(_game->getMod(), rule, _base);
					t->setSoldier(_game->getMod()->genSoldier(_game->getSavedGame(), rule, nationality));
					_base->getTransfers()->push_back(t);
					_faction->getStaffContainer()->removeItem(rule->getType());
				}
				break;
			case TRANSFER_SCIENTIST:
				t = new Transfer(_game->getMod()->getPersonnelTime());
				t->setScientists(i->amount);
				_base->getTransfers()->push_back(t);
				_faction->getStaffContainer()->removeItem("STR_SCIENTIST", i->amount);
				break;
			case TRANSFER_ENGINEER:
				t = new Transfer(_game->getMod()->getPersonnelTime());
				t->setEngineers(i->amount);
				_base->getTransfers()->push_back(t);
				_faction->getStaffContainer()->removeItem("STR_ENGINEER", i->amount);
				break;
			case TRANSFER_CRAFT:
				for (int c = 0; c < i->amount; c++)
				{
					RuleCraft *rule = (RuleCraft*)i->rule;
					t = new Transfer(rule->getTransferTime());
					Craft *craft = new Craft(rule, _base, _game->getSavedGame()->getId(rule->getType()));
					craft->initFixedWeapons(_game->getMod());
					craft->setStatus("STR_REFUELLING");
					t->setCraft(craft);
					_base->getTransfers()->push_back(t);
					_faction->getStaffContainer()->removeItem(rule->getType());
				}
				break;
			case TRANSFER_ITEM:
				{
					RuleItem *rule = (RuleItem*)i->rule;
					t = new Transfer(rule->getTransferTime());
					t->setItems(rule->getType(), i->amount);
					_base->getTransfers()->push_back(t);

					_faction->removeItem(rule, i->amount);
				}
				break;
			}
		}
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the item by one on left-click,
 * to max on right-click.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) increaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		increaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the item by one on left-click,
 * to 0 on right-click.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) decreaseByValue(INT_MAX);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		decreaseByValue(1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void DiplomacyPurchaseState::lstItemsMousePress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			increaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			decreaseByValue(Options::changeValueByMouseWheel);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
		else if (getRow().type == TRANSFER_CRAFT)
		{
			RuleCraft *rule = (RuleCraft*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			return;
		}
		std::string itemName;
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				itemName = rule->getType();
			}
		}
		else if (getRow().type == TRANSFER_CRAFT)
		{
			RuleCraft *rule = (RuleCraft*)getRow().rule;
			if (rule != 0)
			{
				itemName = rule->getType();
			}
		}
		if (!itemName.empty())
		{
			std::map<std::string, bool> hiddenMap = _game->getSavedGame()->getHiddenPurchaseItems();
			std::map<std::string, bool>::iterator iter = hiddenMap.find(itemName);
			if (iter != hiddenMap.end())
			{
				// found => flip it
				_game->getSavedGame()->setHiddenPurchaseItemsStatus(itemName, !iter->second);
			}
			else
			{
				// not found = not hidden yet => hide it
				_game->getSavedGame()->setHiddenPurchaseItemsStatus(itemName, true);
			}

			// update screen
			size_t scrollPos = _lstItems->getScroll();
			updateList();
			_lstItems->scrollTo(scrollPos);
		}
	}
}

/**
 * Increases the quantity of the selected item to buy by one.
 */
void DiplomacyPurchaseState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	increaseByValue(1);
}

/**
 * Increases the quantity of the selected item to buy by "change".
 * @param change How much we want to add.
 */
void DiplomacyPurchaseState::increaseByValue(int change)
{
	if (0 >= change) return;
	std::string errorMessage;

	if (_total + getRow().cost > _game->getSavedGame()->getFunds())
	{
		errorMessage = tr("STR_NOT_ENOUGH_MONEY");
	}
	else if (getRow().amount + 1 > getRow().stock)
	{
		errorMessage = tr("STR_NOT_ENOUGH_STOCK");
	}
	else
	{
		RuleItem *rule = nullptr;
		switch (getRow().type)
		{
		case TRANSFER_SOLDIER:
		case TRANSFER_SCIENTIST:
		case TRANSFER_ENGINEER:
			if (_pQty + 1 > _base->getAvailableQuarters() - _base->getUsedQuarters())
			{
				errorMessage = tr("STR_NOT_ENOUGH_LIVING_SPACE");
			}
			break;
		case TRANSFER_CRAFT:
			if (_cQty + 1 > _base->getAvailableHangars() - _base->getUsedHangars())
			{
				errorMessage = tr("STR_NO_FREE_HANGARS_FOR_PURCHASE");
			}
			break;
		case TRANSFER_ITEM:
			rule = (RuleItem*)getRow().rule;
			if (_iQty + rule->getSize() > _base->getAvailableStores() - _base->getUsedStores())
			{
				errorMessage = tr("STR_NOT_ENOUGH_STORE_SPACE");
			}
			else if (rule->isAlien())
			{
				int p = rule->getPrisonType();
				if (_iPrisonQty[p] + 1 > _base->getAvailableContainment(p) - _base->getUsedContainment(p))
				{
					errorMessage = trAlt("STR_NOT_ENOUGH_PRISON_SPACE", p);
				}
			}
			break;
		}
	}

	if (errorMessage.empty())
	{
		auto row = getRow();
		int maxByMoney = (_game->getSavedGame()->getFunds() - _total) / getRow().cost;
		if (maxByMoney >= 0)
			change = std::min(maxByMoney, change);
		int amount = getRow().amount;
		int stock = getRow().stock;
		if (stock - amount - change < 0)
		{
			change = stock - amount;
		}
		switch (getRow().type)
		{
		case TRANSFER_SOLDIER:
		case TRANSFER_SCIENTIST:
		case TRANSFER_ENGINEER:
			{
				int maxByQuarters = _base->getAvailableQuarters() - _base->getUsedQuarters() - _pQty;
				change = std::min(maxByQuarters, change);
				_pQty += change;
			}
			break;
		case TRANSFER_CRAFT:
			{
				int maxByHangars = _base->getAvailableHangars() - _base->getUsedHangars() - _cQty;
				change = std::min(maxByHangars, change);
				_cQty += change;
			}
			break;
		case TRANSFER_ITEM:
			{
				RuleItem *rule = (RuleItem*)getRow().rule;
				int p = rule->getPrisonType();
				if (rule->isAlien())
				{
					int maxByPrisons = _base->getAvailableContainment(p) - _base->getUsedContainment(p) - _iPrisonQty[p];
					change = std::min(maxByPrisons, change);
				}
				// both aliens and items
				{
					double storesNeededPerItem = rule->getSize();
					double freeStores = _base->getAvailableStores() - _base->getUsedStores() - _iQty;
					double maxByStores = (double)(INT_MAX);
					if (!AreSame(storesNeededPerItem, 0.0) && storesNeededPerItem > 0.0)
					{
						maxByStores = (freeStores + 0.05) / storesNeededPerItem;
					}
					change = std::min((int)maxByStores, change);
					_iQty += change * storesNeededPerItem;
				}
				if (rule->isAlien())
				{
					_iPrisonQty[p] += change;
				}
			}
			break;
		}
		getRow().amount += change;
		_total += getRow().cost * change;
		updateItemStrings();
	}
	else
	{
		_timerInc->stop();
		RuleInterface *menuInterface = _game->getMod()->getInterface("buyMenu");
		_game->pushState(new ErrorMessageState(errorMessage, _palette, menuInterface->getElement("errorMessage")->color, "BACK13.SCR", menuInterface->getElement("errorPalette")->color));
	}
}

/**
 * Decreases the quantity of the selected item to buy by one.
 */
void DiplomacyPurchaseState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	decreaseByValue(1);
}

/**
 * Decreases the quantity of the selected item to buy by "change".
 * @param change how much we want to add.
 */
void DiplomacyPurchaseState::decreaseByValue(int change)
{
	if (0 >= change || 0 >= getRow().amount) return;
	change = std::min(getRow().amount, change);

	RuleItem *rule = nullptr;
	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		_pQty -= change;
		break;
	case TRANSFER_CRAFT:
		_cQty -= change;
		break;
	case TRANSFER_ITEM:
		rule = (RuleItem*)getRow().rule;
		_iQty -= rule->getSize() * change;
		if (rule->isAlien())
		{
			_iPrisonQty[rule->getPrisonType()] -= change;
		}
		break;
	}
	getRow().amount -= change;
	_total -= getRow().cost * change;
	updateItemStrings();
}

/**
 * Updates the quantity-strings of the selected item.
 */
void DiplomacyPurchaseState::updateItemStrings()
{
	_txtPurchases->setText(tr("STR_COST_OF_PURCHASES").arg(Unicode::formatFunding(_total)));
	std::ostringstream ss, ss5;
	ss << getRow().amount;
	_lstItems->setCellText(_sel, 3, ss.str());
	if (getRow().amount > 0)
	{
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	}
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _ammoColor);
			}
		}
	}
	ss5 << _base->getUsedStores();
	if (std::abs(_iQty) > 0.05)
	{
		ss5 << "(";
		if (_iQty > 0.05)
			ss5 << "+";
		ss5 << std::fixed << std::setprecision(1) << _iQty << ")";
	}
	ss5 << ":" << _base->getAvailableStores();
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss5.str()));
}

/**
 * Updates the production list to match the category filter.
 */
void DiplomacyPurchaseState::cbxCategoryChange(Action *)
{
	updateList();
}
/**
 * Returns a value of available item for current reputation level
 * @param item type.
 * @return number of allowed to purchase items.
 */
int DiplomacyPurchaseState::getFactionItemStock(std::string entityName)
{
	// first we look through item storage, we check if entity is an item
	RuleItem* itemRule = _game->getMod()->getItem(entityName);
	if (itemRule)
	{
		int iQty = _faction->getPublicItems()->getItem(itemRule);
		if (iQty > 0)
		{
			if (!itemRule->getReputationRequirements().empty())
			{
				for (auto i : itemRule->getReputationRequirements())
				{
					if (i.first == _faction->getRules()->getName())
					{
						if (_faction->getReputationLevel() < i.second)
						{
							return 0;
						}
					}
				}
			}
			return iQty;
		}
		else
		{
			return 0;
		}
	}
	// if not, we check other factional property
	else
	{
		int sQty = _faction->getStaffContainer()->getItem(entityName);
		if (sQty > 0)
		{
			return sQty;
		}
		else
		{
			return 0;
		}
	}
	// nothing to sell, sorry
	return 0;
}

}
