/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "MasterMind.h"
#include <sstream>
#include <iomanip>
#include <climits>
#include <functional>
#include "../Engine/RNG.h"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Exception.h"
#include "../Engine/State.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Language.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Mod/RuleEventScript.h"
#include "../Mod/RuleEvent.h"
#include "../Mod/RuleResearch.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleAlienMission.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/RuleDiplomacyFaction.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/DiplomacyFaction.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/Region.h"
#include "../Savegame/Country.h"
#include "../Savegame/AlienStrategy.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/FactionalContainer.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"
#include "../fmath.h"

namespace OpenXcom
{

MasterMind::MasterMind(Game* engine): _game(engine)
{
	
}
MasterMind::~MasterMind()
{
}

/**
* Handle additional operations in new FTA game generation.
* @param eventName - string with rules name of the event.
* @return true is event was generater successfully.
*/
void MasterMind::newGameHelper(int diff, GeoscapeState* gs)
{
	SavedGame* save = _game->getSavedGame();
	Mod* mod = _game->getMod();
	Base* base = save->getBases()->at(0);
	double lon, lat;
	lon = RNG::generate(0.20, 0.22); //#FINNIKTODO random array here
	lat = RNG::generate(-0.832, -0.87) ; //#FINNIKTODO random array here
	base->setLongitude(lon);
	base->setLatitude(lat);
	std::string baseName = _game->getLanguage()->getString("STR_LAST_STAND"); //#FINNIKTODO random array here
	base->setName(baseName);
	gs->getGlobe()->center(lon, lat);

	for (std::vector<Craft*>::iterator i = base->getCrafts()->begin(); i != base->getCrafts()->end(); ++i)
	{
		(*i)->setLongitude(lon);
		(*i)->setLatitude(lat);
	}
	//spawn regional MIB HQ
	AlienDeployment* aBaseDeployment = mod->getDeployment("STR_INITIAL_REGIONAL_HQ");
	AlienBase* aBase = new AlienBase(aBaseDeployment, 0);
	aBase->setId(save->getId(aBaseDeployment->getMarkerName()));
	aBase->setAlienRace(aBaseDeployment->getRace());
	aBase->setLongitude(lon + RNG::generate(0.20, 0.26)); //#FINNIKTODO random array here
	aBase->setLatitude(lat - RNG::generate(0.04, 0.06)); //#FINNIKTODO random array here
	aBase->setDiscovered(false);
	save->getAlienBases()->push_back(aBase);

	//init the Game
	gs->init();

	//init Factions
	for (std::vector<std::string>::const_iterator i = mod->getDiplomacyFactionList()->begin(); i != mod->getDiplomacyFactionList()->end(); ++i)
	{
		RuleDiplomacyFaction* factionRules = mod->getDiplomacyFaction(*i);
		DiplomacyFaction* faction = new DiplomacyFaction(mod, factionRules->getName());

		if (factionRules->getDiscoverResearch().empty() || save->isResearched(mod->getResearch(factionRules->getDiscoverResearch())))
		{
			faction->setDiscovered(true);
		}

		// set up starting values
		faction->setReputationScore(factionRules->getStartingReputation());
		updateReputationLvl(faction, true);
		faction->setFunds(factionRules->getStartingFunds());
		faction->setPower(factionRules->getStartingPower()); //we always start with 0 vigilance
		for (auto research : factionRules->getStartingResearches())
		{
			faction->unlockResearch(research);
		}

		// populate faction item stores and staff
		auto items = faction->getPublicItems();
		for (auto &item : factionRules->getStartingItems())
		{
			RuleItem* itemRule = _game->getMod()->getItem(item.first);
			if (itemRule)
			{
				items->addItem(itemRule, item.second);
			}
			else
			{
				throw Exception("Error in FTA game initialization process: fails to add item " + item.first + " for faction " + factionRules->getName() +
					" ; no item ruleset defined!");
			}
		}
		for (auto &staff : factionRules->getStartingStaff())
		{
			faction->getStaffContainer()->addItem(staff.first, staff.second);
		}

		// finish faction initialization process
		save->getDiplomacyFactions().push_back(faction);
	}

	//adjust funding
	int funds = mod->getInitialFunding();
	funds = funds * 1000 + static_cast<int>(RNG::generate(-1258, 6365)); 
	save->setFunds(funds);

	//start base defense mission
	SavedBattleGame* bgame = new SavedBattleGame(mod, _game->getLanguage());
	_game->getSavedGame()->setBattleGame(bgame);
	bgame->setMissionType("STR_BASE_DEFENSE");
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);
	bgen.setBase(base);
	bgen.setAlienCustomDeploy(mod->getDeployment("STR_INITIAL_BASE_DEFENSE"));
	bgen.setWorldShade(0);
	bgen.run();
	_game->pushState(new BriefingState(0, base));
}

/**
* Process event script from different sources
* @param scripts - a vector of event script's string IDs.
* @param source is the reason we are running process (monthly, factional, xcom).
*/
void MasterMind::eventScriptProcessor(std::vector<std::string> scripts, ProcessorSource source)
{
	if (!scripts.empty())
	{
		const Mod& mod = *_game->getMod();
		SavedGame &save = *_game->getSavedGame();
		AlienStrategy &strategy = save.getAlienStrategy();
		std::set<std::string> xcomBaseCountries;
		std::set<std::string> xcomBaseRegions;

		for (auto &xcomBase : *save.getBases())
		{
			auto region = save.locateRegion(*xcomBase);
			if (region)
			{
				xcomBaseRegions.insert(region->getRules()->getType());
			}
			auto country = save.locateCountry(*xcomBase);
			if (country)
			{
				xcomBaseCountries.insert(country->getRules()->getType());
			}
		}

		for (auto& name : scripts)
		{
			auto ruleScript = mod.getEventScript(name);
			int allowedProcessor = ruleScript->getAllowedProcessor();
			// check allowed processor first!
			if ((source == SCRIPT_MONTHLY && allowedProcessor != 0) ||
				(source == SCRIPT_FACTIONAL && allowedProcessor != 1) ||
				(source == SCRIPT_XCOM && allowedProcessor != 2))
			{
				continue; //we should skip that!
			}
			auto month = save.getMonthsPassed();
			int loyalty = save.getLoyalty();
			if (ruleScript->getFirstMonth() <= month &&
				(ruleScript->getLastMonth() >= month || ruleScript->getLastMonth() == -1) &&
				(ruleScript->getMinScore() <= save.getCurrentScore(month)) &&
				(ruleScript->getMaxScore() >= save.getCurrentScore(month)) &&
				(ruleScript->getMinLoyalty() <= loyalty) &&
				(ruleScript->getMaxLoyalty() >= loyalty) &&
				(ruleScript->getMinFunds() <= save.getFunds()) &&
				(ruleScript->getMaxFunds() >= save.getFunds()) &&
				ruleScript->getMinDifficulty() <= save.getDifficulty() &&
				ruleScript->getMaxDifficulty() >= save.getDifficulty() &&
				!save.getEventScriptGapped(ruleScript->getType()))
			{
				// level two condition check: make sure we meet any research requirements, if any.
				bool triggerHappy = true;
				for (std::map<std::string, bool>::const_iterator j = ruleScript->getResearchTriggers().begin(); triggerHappy && j != ruleScript->getResearchTriggers().end(); ++j)
				{
					triggerHappy = (save.isResearched(j->first) == j->second);
					if (!triggerHappy)
						continue;
				}

				// reputation requirements
				if (triggerHappy)
				{
					if (!ruleScript->getReputationRequirments().empty())
					{
						triggerHappy = false;
						for (auto& triggerFaction : ruleScript->getReputationRequirments())
						{
							for (auto& faction : save.getDiplomacyFactions())
							{
								if (faction->getRules()->getName() == triggerFaction.first)
								{
									if (faction->getReputationLevel() >= triggerFaction.second)
									{
										triggerHappy = true;
									}
								}
							}
						}
						if (!triggerHappy)
							continue;
					}
				}
				if (triggerHappy)
				{
					// check counters
					if (ruleScript->getCounterMin() > 0)
					{
						if (!ruleScript->getMissionVarName().empty() && ruleScript->getCounterMin() > strategy.getMissionsRun(ruleScript->getMissionVarName()))
						{
							triggerHappy = false;
						}
						if (!ruleScript->getMissionMarkerName().empty() && ruleScript->getCounterMin() > save.getLastId(ruleScript->getMissionMarkerName()))
						{
							triggerHappy = false;
						}
					}
					if (triggerHappy && ruleScript->getCounterMax() != -1)
					{
						if (!ruleScript->getMissionVarName().empty() && ruleScript->getCounterMax() < strategy.getMissionsRun(ruleScript->getMissionVarName()))
						{
							triggerHappy = false;
						}
						if (!ruleScript->getMissionMarkerName().empty() && ruleScript->getCounterMax() < save.getLastId(ruleScript->getMissionMarkerName()))
						{
							triggerHappy = false;
						}
					}
				}
				if (triggerHappy)
				{
					// item requirements
					for (auto& triggerItem : ruleScript->getItemTriggers())
					{
						triggerHappy = (save.isItemObtained(triggerItem.first) == triggerItem.second);
						if (!triggerHappy)
							continue;
					}
				}
				if (triggerHappy)
				{
					// facility requirements
					for (auto& triggerFacility : ruleScript->getFacilityTriggers())
					{
						triggerHappy = (save.isFacilityBuilt(triggerFacility.first) == triggerFacility.second);
						if (!triggerHappy)
							continue;
					}
				}
				if (triggerHappy)
				{
					// xcom base requirements by region
					for (auto &triggerXcomBase : ruleScript->getXcomBaseInRegionTriggers())
					{
						bool found = (xcomBaseRegions.find(triggerXcomBase.first) != xcomBaseRegions.end());
						triggerHappy = (found == triggerXcomBase.second);
						if (!triggerHappy)
							break;
					}
				}
				if (triggerHappy)
				{
					// xcom base requirements by country
					for (auto &triggerXcomBase2 : ruleScript->getXcomBaseInCountryTriggers())
					{
						bool found = (xcomBaseCountries.find(triggerXcomBase2.first) != xcomBaseCountries.end());
						triggerHappy = (found == triggerXcomBase2.second);
						if (!triggerHappy)
							break;
					}
				}
				// ok, we still want event from this script, now let`s actually choose one.
				if (triggerHappy)
				{
					std::vector<const RuleEvent*> toBeGenerated;

					// 1. sequentially generated one-time events (cannot repeat)
					{
						std::vector<std::string> possibleSeqEvents;
						for (auto& seqEvent : ruleScript->getOneTimeSequentialEvents())
						{
							if (!save.wasEventGenerated(seqEvent))
								possibleSeqEvents.push_back(seqEvent); // insert
						}
						if (!possibleSeqEvents.empty())
						{
							auto eventRules = mod.getEvent(possibleSeqEvents.front(), true); // take first
							toBeGenerated.push_back(eventRules);
						}
					}

					// 2. randomly generated one-time events (cannot repeat)
					{
						WeightedOptions possibleRngEvents;
						WeightedOptions tmp = ruleScript->getOneTimeRandomEvents(); // copy for the iterator, because of getNames()
						possibleRngEvents = tmp; // copy for us to modify
						for (auto& rngEvent : tmp.getNames())
						{
							if (save.wasEventGenerated(rngEvent))
								possibleRngEvents.set(rngEvent, 0); // delete
						}
						if (!possibleRngEvents.empty())
						{
							auto eventRules = mod.getEvent(possibleRngEvents.choose(), true); // take random
							toBeGenerated.push_back(eventRules);
						}
					}

					// 3. randomly generated repeatable events
					{
						auto eventRules = mod.getEvent(ruleScript->generate(save.getMonthsPassed()), false);
						if (eventRules)
						{
							toBeGenerated.push_back(eventRules);
						}
					}

					// 4. generate
					bool generated = false;
					for (auto eventRules : toBeGenerated)
					{
						if (save.spawnEvent(eventRules))
						{
							generated = true;
						}
					}
					// 4a. if needed any of events were generated, we mark this with gap timer.
					if (generated)
					{
						int timer = ruleScript->getSpawnGap();
						timer += RNG::generate(0, ruleScript->getRandomSpawnGap());
						if (timer > 0)
						{
							_game->getSavedGame()->setEventScriptGapTimer(ruleScript->getType(), timer);
						}
					}
				}
			}
		}
	}
}

bool MasterMind::spawnAlienMission(const std::string& missionName, const Globe& globe, Base* base)
{
	// let's define variables for alien mission first
	const Mod& mod = *_game->getMod();
	SavedGame &save = *_game->getSavedGame();
	int month = save.getMonthsPassed();
	bool success = true;
	const RuleAlienMission* missionRules = mod.getAlienMission(missionName); // would work with string ID 'till OXCE make mission rules afterloading
	if (missionRules == 0)
	{
		throw Exception("Error processing alien mission generation - rules for alienMission: " + missionName + " are not defined!");
	}
	std::string targetRegion;
	std::string missionRace;
	int targetZone = missionRules->getSpawnZone();

	//bool isSiteType = missionRules->getObjective() == OBJECTIVE_SITE;
	bool targetBase = RNG::percent(missionRules->getTargetBaseOdds());
	bool placed = false;
	bool baseTargeted = true;
	bool hasZone = true;
	int tries = 0;
		
	while (!placed || tries < 50)
	{
		if (missionRules->hasRegionWeights())
		{
			if (tries > 35) month = 0; //lets check all regions
			targetRegion = missionRules->generateRegion(month);
		}
		else //case to pick at random as alien mission rules has no region defined
		{
			targetRegion = mod.getRegionsList().at(RNG::generate(0, mod.getRegionsList().size() - 1));
		}

		RuleRegion* region = mod.getRegion(targetRegion, true);
		if (!region)
		{
			throw Exception("Error processing alien mission named: " + missionName + ", region named: " + targetRegion + " is not defined");
		}
		if ((int)(region->getMissionZones().size()) > targetZone)
		{
			hasZone = true;
		}

		if (targetBase && base) // we should choose region that has any xcom base
		{
			baseTargeted = false;
			std::string baseRegion;
			if (tries < 2)
			{
				baseRegion = save.locateRegion(base->getLongitude(), base->getLatitude())->getRules()->getType();
				if (baseRegion == targetRegion)
				{
					baseTargeted = true;
				}
			}
			else
			{
				for (std::vector<Base*>::iterator i = save.getBases()->begin(); i != save.getBases()->end(); ++i)
				{
					baseRegion = save.locateRegion((*i)->getLongitude(), (*i)->getLatitude())->getRules()->getType();
					if (baseRegion == targetRegion)
					{
						baseTargeted = true;
					}
				}
			}
		}
		tries++;
		if (baseTargeted && hasZone) // all checks passed!
		{
			placed = true;
		}
	}

	if(!placed)
	{
		Log(LOG_ERROR) << "An error occurred during the processing alien mission spawning! Failed to choose right region for mission: " << missionName <<
			". Some alien mission rules could be ignored!";
		success = false;
	}

	missionRace = missionRules->generateRace(month);
	if (missionRace.empty())
	{
		if (mod.isFTAGame())
		{
			missionRace = "STR_MIB";
			Log(LOG_ERROR) << "An error occurred during the processing alien mission spawning! In the rules of the alien mission: " << missionName <<
				" no alien race has been set! As we run FTAGame race set to " << missionRace;
			success = false;
		}
		else
		{
			Log(LOG_ERROR) << "An error occurred during the processing alien mission spawning! In the rules of the alien mission: " << missionName <<
				" no alien race has been set, so it will be defined at random!";
			auto raceList = mod.getAlienRacesList();
			int pick = RNG::generate(0, raceList.size() - 1);
			missionRace = raceList.at(pick);
			success = false;
		}
	}
	if (mod.getAlienRace(missionRace) == 0)
	{
		throw Exception("Error processing alien mission named: " + missionName + ", race: " + missionRace + " is not defined");
	}

	//now we are ready to set up new alien mission
	AlienMission* mission = new AlienMission(*missionRules);
	mission->setRace(missionRace); 
	mission->setId(save.getId("ALIEN_MISSIONS"));
	mission->setRegion(targetRegion, mod);
	mission->setMissionSiteZone(targetZone);
	mission->start(*_game, globe, 0);
	save.getAlienMissions().push_back(mission);

	return success;
}

int MasterMind::updateLoyalty(int score, LoyaltySource source)
{
	if (!_game->getMod()->isFTAGame())
	{
		return 0;
	}
	double coef = 1;
	std::string reason = "";
	switch (source)
	{
	case XCOM_BATTLESCAPE:
		coef = _game->getMod()->getLoyaltyCoefBattlescape();
		reason = "XCOM_BATTLESCAPE";
		break;
	case XCOM_DOGFIGHT:
		coef = _game->getMod()->getLoyaltyCoefDogfight();
		reason = "XCOM_DOGFIGHT";
		break;
	case XCOM_GEOSCAPE:
		coef = _game->getMod()->getLoyaltyCoefGeoscape();
		reason = "XCOM_GEOSCAPE";
		break;
	case XCOM_RESEARCH:
		coef = _game->getMod()->getLoyaltyCoefResearch();
		reason = "XCOM_RESEARCH";
		break;
	case ALIEN_MISSION_DESPAWN:
		coef = -_game->getMod()->getLoyaltyCoefAlienMission();
		reason = "ALIEN_MISSION_DESPAWN";
		break;
	case ALIEN_UFO_ACTIVITY:
		coef = -_game->getMod()->getLoyaltyCoefUfo();
		reason = "ALIEN_UFO_ACTIVITY";
		break;
	case ALIEN_BASE:
		coef = -_game->getMod()->getLoyaltyCoefAlienBase();
		reason = "ALIEN_BASE";
		break;
	case ABSOLUTE_COEF:
		coef = 100;
		reason = "ABSOLUTE";
		break;
	}

	coef /= 100;
	int change = std::round(coef * score);
	int loyalty = _game->getSavedGame()->getLoyalty();
	loyalty += change;
	Log(LOG_DEBUG) << "Loyalty updating to:  " << loyalty << " from coef: " << coef << ", change value: " << change << " and score value : " << score << " with reason : " << reason; //#CLEARLOGS
	_game->getSavedGame()->setLoyalty(loyalty);

	return change;
}

/**
* Handle calculation of base services (manufacture, labs and craft repair) performance bonus caused by loyalty score.
* @return value of performance bonus, normal value is 100 %.
*/
int MasterMind::getLoyaltyPerformanceBonus()
{
	int performance = 100;
	if (_game->getMod()->isFTAGame())
	{
		int loyalty = _game->getSavedGame()->getLoyalty();
		if (loyalty > 100) // function approximation goes weird on values from 0 to 100, so we just keep it bonusless there...
		{
			double ln = log(std::abs(loyalty));
			int bonus = ceil(-9.79 + (2.23 * ln));
			performance += bonus;
		}
		else if (loyalty < 0)
		{
			int penalty = ceil(0.271 * pow(-loyalty, 0.537));
			performance -= penalty;
		}
	}
	return performance;
}

/**
* Handle updating of faction reputation level based on current score, rules and other conditions.
* @param faction - DiplomacyFaction we are updating.
* @param initial - to define if we are performing updating of reputation due to new game initialisation (default false).
* @return true if reputation level was updated.
*/
bool MasterMind::updateReputationLvl(DiplomacyFaction* faction, bool initial)
{
	int repScore = faction->getReputationScore();
	int curLvl = faction->getReputationLevel();
	bool changed = false;
	std::string repName = "STR_NEUTRAL";
	int newLvl = 3; //STR_NEUTRAL is default resolve

	const std::map<int, std::string>* repLevels = _game->getMod()->getReputationLevels();
	if (repLevels)
	{
		int temp = INT_MIN;
		for (auto& i : *repLevels)
		{
			if (i.first > temp && i.first <= repScore)
			{
				temp = i.first;
				repName = i.second;
			}
		}

		if (repName == "STR_ALLY") newLvl = 6;
		else if (repName == "STR_HONORED") newLvl = 5;
		else if (repName == "STR_FRIENDLY") newLvl = 4;
		else if (repName == "STR_UNFRIENDLY") newLvl = 2;
		else if (repName == "STR_HOSTILE") newLvl = 1;
		else if (repName == "STR_HATED") newLvl = 0;

		if ((curLvl != newLvl && !faction->isThisMonthDiscovered()) || initial)
		{
			faction->setReputationLevel(newLvl);
			faction->setReputationName(repName);
			if (!initial)
			{
				changed = true;
				//#FINNIKTODO add cross-factional relations
			}
		}
	}

	return changed;
}

/**
 * Updates nessesary data to process unlocking multiple researches.
 * 
 */
void MasterMind::helpResearchDiscovery(std::vector<const RuleResearch*> projects, std::vector<const RuleResearch*> &possibilities, Base* base, std::string& researchName, std::string& bonusResearchName)
{
	auto mod = _game->getMod();
	auto save = _game->getSavedGame();

	for (auto rRule : projects)
	{
		if (!save->isResearched(rRule, false) || save->hasUndiscoveredGetOneFree(rRule, true))
		{
			possibilities.push_back(rRule);
		}
	}

	std::vector<const RuleResearch*> topicsToCheck;
	if (!possibilities.empty())
	{
		size_t pickResearch = RNG::generate(0, possibilities.size() - 1);
		const RuleResearch* research = possibilities.at(pickResearch);

		//we also delete chosen research from possibility list for future use
		std::vector<const RuleResearch*>::const_iterator it = std::find(possibilities.begin(), possibilities.end(), research);
		if (it != possibilities.end())
		{
			possibilities.erase(it);
		}
		

		bool alreadyResearched = false;
		std::string name = research->getLookup().empty() ? research->getName() : research->getLookup();
		if (save->isResearched(name, false))
		{
			alreadyResearched = true; // we have seen the pedia article already, don't show it again
		}

		save->addFinishedResearch(research, mod, base, true);
		topicsToCheck.push_back(research);
		researchName = alreadyResearched ? "" : research->getName();

		if (!research->getLookup().empty())
		{
			const RuleResearch* lookupResearch = mod->getResearch(research->getLookup(), true);
			save->addFinishedResearch(lookupResearch, mod, base, true);
			researchName = alreadyResearched ? "" : lookupResearch->getName();
		}

		if (auto bonus = save->selectGetOneFree(research))
		{
			save->addFinishedResearch(bonus, mod, base, true);
			topicsToCheck.push_back(bonus);
			bonusResearchName = bonus->getName();

			if (!bonus->getLookup().empty())
			{
				const RuleResearch* bonusLookup = mod->getResearch(bonus->getLookup(), true);
				save->addFinishedResearch(bonusLookup, mod, base, true);
				bonusResearchName = bonusLookup->getName();
			}
		}
	}

	save->handlePrimaryResearchSideEffects(topicsToCheck, mod, base);
}

}
