/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos-zero>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/SingletonImp.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "extras/Mod.h"

#define NULL_AURA_SLOT 0xFF

pAuraHandler AuraHandler[TOTAL_AURAS]=
{
    &Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
    &Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonusTaken and Unit::SpellBaseDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD      implemented in Unit::DealMeleeDamage
    &Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT
    &Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
    &Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
    &Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &Aura::HandleUnused,                                    // 46 SPELL_AURA_46
    &Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Aura::HandleUnused,                                    // 48 SPELL_AURA_48
    &Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Aura::HandleUnused,                                    // 50 SPELL_AURA_MOD_BLOCK_SKILL    obsolete?
    &Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
    &Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonusDone and Unit::SpellDamageBonusDone
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 SPELL_AURA_PERIODIC_MANA_FUNNEL obsolete?
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell that has only visual effect
    &Aura::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Aura::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
    &Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonusTaken and Unit::SpellDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 SPELL_AURA_MOD_RESIST_CHANCE  Useless
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleAurasVisible,                              //100 SPELL_AURA_AURAS_VISIBLE
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleUnused,                                    //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonusTaken
    &Aura::HandleUnused,                                    //119 SPELL_AURA_SHARE_PET_TRACKING useless
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleNoImmediateEffect,                         //123 SPELL_AURA_MOD_TARGET_RESISTANCE  implemented in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonusDone
    &Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Aura::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
    &Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Aura::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &Aura::HandleUnused,                                    //145 SPELL_AURA_CHARISMA obsolete?
    &Aura::HandleUnused,                                    //146 SPELL_AURA_PERSUADED obsolete?
    &Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK            implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect (check part)
    &Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_RESIST_PUSHBACK
    &Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE         implemented in Creature::GetAttackDistance
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::getMaxTimer
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
    &Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE                 implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleUnused,                                    //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
    &Aura::HandleUnused,                                    //164 useless, only one test spell
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonusDone, Unit::MeleeDamageBonusDone
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleDetectAmore,                               //170 SPELL_AURA_DETECT_AMORE       only for Detect Amore spell
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonusDone (in 1.12.* only spirit)
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonusDone (in 1.12.* only spirit)
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonusDone
    &Aura::HandleUnused,                                    //181 SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS unused
    &Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::CalcThreat
    &Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &Aura::HandleUnused,                                    //189 SPELL_AURA_MOD_RATING (not used in 1.12.1)
    &Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* /*castItem*/) :
    m_spellmod(NULL), m_periodicTimer(0), m_periodicTick(0), m_removeMode(AURA_REMOVE_BY_DEFAULT),
    m_effIndex(eff), m_positive(false), m_isPeriodic(false), m_isAreaAura(false), m_isPersistent(false), m_pulseResistFrequency(0), m_pulseResistTimer(0),m_isPvp(false),
    m_pulseResistCount(0), m_in_use(0), m_spellAuraHolder(holder)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : spellproto->CalculateSimpleValue(eff);

    m_positive = IsPositiveEffect(spellproto, m_effIndex);
    m_applyTime = time(NULL);

    int32 damage;
    if (!caster)
        damage = m_currentBasePoints;
    else
    {
        damage = caster->CalculateSpellDamage(target, spellproto, m_effIndex, &m_currentBasePoints);
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura: construct Spellid : %u, Aura : %u Target : %d Damage : %d", spellproto->Id, spellproto->EffectApplyAuraName[eff], spellproto->EffectImplicitTargetA[eff], damage);

    SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff]);

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    // Apply periodic time mod
    if (modOwner && m_modifier.periodictime)
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);

    // Start periodic on next tick
    m_periodicTimer += m_modifier.periodictime;
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                   Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellproto->EffectRadiusIndex[m_effIndex]));
    if(Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_RADIUS, m_radius);

    switch(spellproto->Effect[eff])
    {
    case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
        m_areaAuraType = AREA_AURA_PARTY;
        if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
            m_modifier.m_auraname = SPELL_AURA_NONE;
        break;
    case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        m_areaAuraType = AREA_AURA_PET;
        break;
    default:
        sLog.outError("Wrong spell effect in AreaAura constructor");
        MANGOS_ASSERT(false);
        break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                                       Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
        Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    if (caster)
        m_castersTargetGuid = caster->GetTypeId()==TYPEID_PLAYER ? ((Player*)caster)->GetSelectionGuid() : caster->GetTargetGuid();
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
    return ObjectAccessor::GetUnit(*(m_spellAuraHolder->GetTarget()), m_castersTargetGuid);
}

CumulativeEffectAura::~CumulativeEffectAura()
{
}

CumulativeEffectAura::CumulativeEffectAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
        Unit *caster, Item* castItem) : SingleEnemyTargetAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_cumulativeEffect = 0;
}

float CumulativeEffectAura::GetCumulativeEffect()
{
    return m_cumulativeEffect;
}

void CumulativeEffectAura::AddCumulativeEffect(float value)
{
    m_cumulativeEffect += value;
}

void CumulativeEffectAura::SetDamageModifier(float value)
{
    m_damageModifier = value;
}

float CumulativeEffectAura::GetDamageModifier()
{
    return m_damageModifier;
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

    return new Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);
}

SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, bool isTriggered, Unit *target, WorldObject *caster, Item *castItem)
{
    return new SpellAuraHolder(spellproto, isTriggered, target, caster, castItem);
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue)
{
    m_modifier.m_auraname = t;
    m_modifier.m_amount = a;
    m_modifier.m_miscvalue = miscValue;
    m_modifier.periodictime = pt;
}

void Aura::Update(uint32 diff)
{
    if (m_pulseResistFrequency == 0 && (GetSpellProto()->Attributes & SPELL_ATTR_HEARTBEAT_RESIST) != 0 && !m_positive && GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER)
    {
        m_isPvp = GetTarget() != NULL && GetTarget()->GetCharmerOrOwnerOrOwnGuid().IsPlayer() && GetCaster() != NULL && GetCaster()->GetCharmerOrOwnerOrOwnGuid().IsPlayer();

        //We have to calculate this stuff here instead of the constructor because we're relying on the DR calcs (which take place after construction) to limit the
        //duration correctly so that our pulse frequency is based on the actual assumed duration of the debuff

        //Live WoW didn't tick more often than 1.5s so abilities can tick LESS often than that (in PVE scenarios if the duration is longer than 15s), but not MORE often
        if (m_isPvp || this->GetAuraMaxDuration() <= 15000)
            m_pulseResistFrequency = 1500;
        else
            m_pulseResistFrequency = this->GetAuraMaxDuration()/10;

        m_pulseResistTimer = m_pulseResistFrequency;
    }

    // Risk of failure for the Goblin Rocket Boots. (This is put here since movement modifying auras do not have proc-handlers.)
    Player* target = dynamic_cast<Player*>(m_spellAuraHolder->GetTarget());

    if (GetSpellProto()->Id == 8892 && target)
    {
        // Give the boots a 2 % risk to break every second that they are turned on.
        uint32 breakage_risk = 2*diff;
        if(urand(1, 100000) <= breakage_risk)
        {
            target->CastSpell(target, 13360, true); // Knock the player down.
            target->CastSpell(target, 8897, false); // Remove the boots and add the scortched ones.
            target->RemoveAura(8892, EFFECT_INDEX_0); // Remove the fast-running aura.
        }
    }
    else if(GetSpellProto()->Id == 13141 && target) // Failure for the Gnomish Rocket Boots.
    {
        // Give the boots a 1 % risk to fail per second.
        uint32 failure_risk = 1*diff;
        if(urand(1, 100000) <= failure_risk)
            target->CastSpell(target, 13158, true);
    }

    //Pulse resist before handling periodic effects, where applicable
    if (m_pulseResistFrequency != 0)
    {
        if (m_pulseResistTimer <= (int)diff)
        {
            m_pulseResistTimer = (m_pulseResistTimer + m_pulseResistFrequency) - diff;

            //Pulse resist
            int pvpBias = 0;

            if (m_isPvp)
            {
                //We should add an increasing PVP bias for the following spells:
                // - Fear
                // - Scare Beast
                // - Polymorph
                // - Sap
                // - Hibernate
                switch (this->GetSpellProto()->Id)
                {
                case 2637:	//Hibernate Rank 1
                case 18657: //Hibernate Rank 2
                case 18658: //Hibernate Rank 3
                case 1513:	//Scare Beast Rank 1
                case 14326:	//Scare Beast Rank 2
                case 14327: //Scare Beast Rank 3
                case 6770:	//Sap Rank 1
                case 2070:	//Sap Rank 2
                case 11297:	//Sap Rank 3
                case 118:	//Polymorph Rank 1
                case 12824:	//Polymorph Rank 2
                case 12825: //Polymorph Rank 3
                case 12826:	//Polymorph Rank 4
                case 5782:	//Fear Rank 1
                case 6213:	//Fear Rank 2
                case 6215:	//Fear Rank 3
                    pvpBias = (m_pulseResistCount * m_pulseResistCount * 100)/2;
                    break;
                default:
                    //do nothing
                    break;
                }
            }

            bool resist = false;

            if (GetCaster() == NULL)
                resist = true;
            else
            {
                switch (GetSpellProto()->DmgClass)
                {
                case SPELL_DAMAGE_CLASS_MAGIC:
                {
                    SpellMissInfo spellInfo = m_spellAuraHolder->GetCaster()->MagicSpellHitResult(m_spellAuraHolder->GetTarget(), GetSpellProto(), true, pvpBias);

                    if (spellInfo != SPELL_MISS_NONE)
                        resist = true;

                    break;
                }
                case SPELL_DAMAGE_CLASS_MELEE:
                case SPELL_DAMAGE_CLASS_RANGED:
                    if (m_spellAuraHolder->GetCaster()->MeleeSpellHitResult(m_spellAuraHolder->GetTarget(), GetSpellProto(), pvpBias) != SPELL_MISS_NONE)
                        resist = true;
                }

                if (!resist && GetSpellProto()->School != SPELL_SCHOOL_NORMAL && GetSpellProto()->School != SPELL_SCHOOL_HOLY)
                {
                    if (m_spellAuraHolder->GetCaster()->MagicSpellBinaryResistResult(m_spellAuraHolder->GetTarget(), GetSpellProto()) != SPELL_MISS_NONE)
                        resist = true;
                }
            }

            if (resist)
            {
                m_spellAuraHolder->GetTarget()->RemoveSpellAuraHolder(m_spellAuraHolder,m_removeMode);
                return;
            }

            m_pulseResistCount++;
        } else
            m_pulseResistTimer -= diff;
    }

    if (m_isPeriodic)
    {
        m_periodicTimer -= diff;
        if(m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;
            ++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
            PeriodicTick();
        }
    }
}

void AreaAura::Update(uint32 diff)
{
    // update for the caster of the aura
    if(GetCasterGuid() == GetTarget()->GetObjectGuid())
    {
        Unit* caster = GetTarget();

        if( !caster->hasUnitState(UNIT_STAT_ISOLATED) )
        {
            Unit* owner = caster->GetCharmerOrOwner();
            if (!owner)
                owner = caster;
            Spell::UnitList targets;

            switch(m_areaAuraType)
            {
            case AREA_AURA_PARTY:
            {
                Group *pGroup = NULL;

                if (owner->GetTypeId() == TYPEID_PLAYER)
                    pGroup = ((Player*)owner)->GetGroup();

                if( pGroup)
                {
                    uint8 subgroup = ((Player*)owner)->GetSubGroup();
                    for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                    {
                        Player* Target = itr->getSource();
                        if(Target && Target->isAlive() && Target->GetSubGroup()==subgroup && caster->IsFriendlyTo(Target))
                        {
                            if(caster->IsWithinDistInMap(Target, m_radius))
                                targets.push_back(Target);
                            Pet *pet = Target->GetPet();
                            if(pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                targets.push_back(pet);
                        }
                    }
                }
                else
                {
                    // add owner
                    if( owner != caster && caster->IsWithinDistInMap(owner, m_radius) )
                        targets.push_back(owner);
                    // add caster's pet
                    Unit* pet = caster->GetPet();
                    if( pet && caster->IsWithinDistInMap(pet, m_radius))
                        targets.push_back(pet);
                }
                break;
            }
            case AREA_AURA_PET:
            {
                if(owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                    targets.push_back(owner);
                break;
            }
            }

            for(Spell::UnitList::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
            {
                // flag for seelction is need apply aura to current iteration target
                bool apply = true;

                // we need ignore present caster self applied are auras sometime
                // in cases if this only auras applied for spell effect
                Unit::SpellAuraHolderBounds spair = (*tIter)->GetSpellAuraHolderBounds(GetId());
                for(Unit::SpellAuraHolderMap::const_iterator i = spair.first; i != spair.second; ++i)
                {
                    if (i->second->IsDeleted())
                        continue;

                    Aura *aur = i->second->GetAuraByEffectIndex(m_effIndex);

                    if (!aur)
                        continue;

                    // in generic case not allow stacking area auras
                    apply = false;
                    break;
                }

                if(!apply)
                    continue;

                if(SpellEntry const *actualSpellInfo = sSpellMgr.SelectAuraRankForLevel(GetSpellProto(), (*tIter)->getLevel()))
                {
                    int32 actualBasePoints = m_currentBasePoints;
                    // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                    if(actualSpellInfo != GetSpellProto())
                        actualBasePoints = actualSpellInfo->CalculateSimpleValue(m_effIndex);

                    SpellAuraHolder *holder = (*tIter)->GetSpellAuraHolder(actualSpellInfo->Id, GetCasterGuid());

                    bool addedToExisting = true;
                    if (!holder)
                    {
                        holder = CreateSpellAuraHolder(actualSpellInfo, false, (*tIter), caster);
                        addedToExisting = false;
                    }

                    holder->SetAuraDuration(GetAuraDuration());

                    AreaAura *aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, holder, (*tIter), caster, NULL);
                    holder->AddAura(aur, m_effIndex);

                    if (addedToExisting)
                    {
                        (*tIter)->AddAuraToModList(aur);
                        holder->SetInUse(true);
                        aur->ApplyModifier(true,true);
                        holder->SetInUse(false);
                    }
                    else
                        (*tIter)->AddSpellAuraHolder(holder);
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        Aura::Update(diff);

        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = true;
        if( !caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
                !caster->IsWithinDistInMap(target, m_radius)        ||
                !caster->HasAura(GetId(), GetEffIndex())            ||
                caster->IsFriendlyTo(target) != needFriendly
          )
        {
            target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
        }
        else if( m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGuid() != target->GetObjectGuid() && caster->GetObjectGuid() != target->GetCharmerOrOwnerGuid())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
                }
                else
                    target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
            }
        }
        else if (m_areaAuraType == AREA_AURA_PET)
        {
            if (target->GetObjectGuid() != caster->GetCharmerOrOwnerGuid())
                target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
        }
    }
}

void PersistentAreaAura::Update(uint32 diff)
{
    bool remove = false;

    // remove the aura if its caster or the dynamic object causing it was removed
    // or if the target moves too far from the dynamic object
    if(Unit *caster = GetCaster())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
        if (dynObj)
        {
            if (!GetTarget()->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
            {
                remove = true;
                dynObj->RemoveAffected(GetTarget());        // let later reapply if target return to range
            }
        }
        else
            remove = true;
    }
    else
        remove = true;

    Aura::Update(diff);

    if(remove)
        GetTarget()->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real)
{
    AuraType aura = m_modifier.m_auraname;

    GetHolder()->SetInUse(true);
    SetInUse(true);
    if(aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);

    sMod.auraApplyModifier(this,aura,apply,Real);
    SetInUse(false);
    GetHolder()->SetInUse(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
    if (m_spellmod)
        return m_spellmod->isAffectedOnSpell(spell);

    // Check family name
    if (spell->SpellFamilyName != GetSpellProto()->SpellFamilyName)
        return false;

    ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(),GetEffIndex());
    return spell->IsFitToFamilyMask(mask);
}

bool Aura::CanProcFrom(SpellEntry const *spell, uint32 EventProcEx, uint32 procEx, bool active, bool useClassMask) const
{
    // Check EffectClassMask (in pre-3.x stored in spell_affect in fact)
    ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());

    // if no class mask defined, or spell_proc_event has SpellFamilyName=0 - allow proc
    if (!useClassMask || !mask)
    {
        if (!(EventProcEx & PROC_EX_EX_TRIGGER_ALWAYS))
        {
            // Check for extra req (if none) and hit/crit
            if (EventProcEx == PROC_EX_NONE)
            {
                // No extra req, so can trigger only for active (damage/healing present) and hit/crit
                if((procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && active)
                    return true;
                else
                    return false;
            }
            else // Passive spells hits here only if resist/reflect/immune/evade
            {
                // Passive spells can`t trigger if need hit (exclude cases when procExtra include non-active flags)
                if ((EventProcEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT) & procEx) && !active)
                    return false;
            }
        }
        return true;
    }
    else
    {
        // SpellFamilyName check is performed in SpellMgr::IsSpellProcEventCanTriggeredBy and it is done once for whole holder
        // note: SpellFamilyName is not checked if no spell_proc_event is defined
        return mask.IsFitToFamilyMask(spell->SpellFamilyFlags);
    }
}

void Aura::ReapplyAffectedPassiveAuras( Unit* target )
{
    // we need store cast item guids for self casted spells
    // expected that not exist permanent auras from stackable auras from different items
    std::map<uint32, ObjectGuid> affectedSelf;

    for(Unit::SpellAuraHolderMap::const_iterator itr = target->GetSpellAuraHolderMap().begin(); itr != target->GetSpellAuraHolderMap().end(); ++itr)
    {
        // permanent passive
        // passive spells can be affected only by own or owner spell mods)
        if (itr->second->IsPassive() && itr->second->IsPermanent() &&
                // non deleted and not same aura (any with same spell id)
                !itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
                // and affected by aura
                itr->second->GetCasterGuid() == target->GetObjectGuid() &&
                // and affected by spellmod
                isAffectedOnSpell(itr->second->GetSpellProto()))
        {
            affectedSelf[itr->second->GetId()] = itr->second->GetCastItemGuid();
        }
    }

    if (!affectedSelf.empty())
    {
        Player* pTarget = target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : NULL;

        for(std::map<uint32, ObjectGuid>::const_iterator map_itr = affectedSelf.begin(); map_itr != affectedSelf.end(); ++map_itr)
        {
            Item* item = pTarget && map_itr->second ? pTarget->GetItemByGuid(map_itr->second) : NULL;
            target->RemoveAurasDueToSpell(map_itr->first);
            target->CastSpell(target, map_itr->first, true, item);
        }
    }
}

struct ReapplyAffectedPassiveAurasHelper
{
    explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
    void operator()(Unit* unit) const {
        aura->ReapplyAffectedPassiveAuras(unit);
    }
    Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
    // not reapply spell mods with charges (use original value because processed and at remove)
    if (GetSpellProto()->procCharges)
        return;

    // not reapply some spell mods ops (mostly speedup case)
    switch (m_modifier.m_miscvalue)
    {
    case SPELLMOD_DURATION:
    case SPELLMOD_CHARGES:
    case SPELLMOD_NOT_LOSE_CASTING_TIME:
    case SPELLMOD_CASTING_TIME:
    case SPELLMOD_COOLDOWN:
    case SPELLMOD_COST:
    case SPELLMOD_ACTIVATION_TIME:
    case SPELLMOD_CASTING_TIME_OLD:
        return;
    }

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(GetTarget());

    // re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
    GetTarget()->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this), CONTROLLED_PET|CONTROLLED_TOTEMS);
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    if(m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;

    if (apply)
    {
        // Add custom charges for some mod aura
        switch (GetSpellProto()->Id)
        {
        case 21879:										// Netherwind threat reduction aura.
            if (m_modifier.m_amount == -2000)
                m_modifier.m_amount = -20;
            else
                m_modifier.m_amount = -100;

            break;

        case 17941:                                     // Shadow Trance
        case 22008:                                     // Netherwind Focus
            GetHolder()->SetAuraCharges(1);
            break;
        }

        // In pre-TBC wrong spellmods in DBC
        switch (GetSpellProto()->SpellIconID)
        {
        case 143:       // Permafrost Speed Decrease
            if (GetEffIndex() == EFFECT_INDEX_1)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
            break;
        case 228:       // Improved Curse of Exhaustion Speed Decrease
            if (GetEffIndex() == EFFECT_INDEX_0)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
            break;
        case 250:       // Camouflage Speed Decrease
            if (GetEffIndex() == EFFECT_INDEX_0)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT3;
            break;
        case 1181:       // Pathfinding Speed Increase
            if (GetEffIndex() == EFFECT_INDEX_0)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
            break;
        case 1494:       // Amplify Curse Speed Decrease
            if (GetEffIndex() == EFFECT_INDEX_1)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
            break;
        case 1563:       // Cheetah Sprint
            if (GetEffIndex() == EFFECT_INDEX_0)
                m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
            break;
        }

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            // prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
            // all this spell expected expire not at use but at spell proc event check
            GetSpellProto()->StackAmount > 1 ? 0 : GetHolder()->GetAuraCharges());
    }
    else
    {
        // If the Combustion modifier is removed so should the dummy aura.
        if (!GetTarget()->HasAura(28682))
        {
            for (uint8 i = 0; i <= EFFECT_INDEX_2; i++)
                GetTarget()->RemoveAura(11129, (SpellEffectIndex) i);
        }

    }


    ((Player*)GetTarget())->AddSpellMod(m_spellmod, apply);

    ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
    ObjectGuid casterGUID = GetCasterGuid();
    Unit* triggerTarget = GetTriggerTarget();

    if (!casterGUID || !triggerTarget)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;
    Unit* target = GetTarget();

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch(auraSpellInfo->SpellFamilyName)
        {
        case SPELLFAMILY_GENERIC:
        {
            switch(auraId)
            {
                // Firestone Passive (1-4 ranks)
            case 758:
            case 17945:
            case 17947:
            case 17949:
            case 27252:
            {
                if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                    return;
                Item* item = ((Player*)triggerTarget)->GetWeaponForAttack(BASE_ATTACK);
                if (!item)
                    return;
                uint32 enchant_id = 0;
                switch (GetId())
                {
                case   758:
                    enchant_id = 1803;
                    break;   // Rank 1
                case 17945:
                    enchant_id = 1823;
                    break;   // Rank 2
                case 17947:
                    enchant_id = 1824;
                    break;   // Rank 3
                case 17949:
                    enchant_id = 1825;
                    break;   // Rank 4
                case 27252:
                    enchant_id = 2645;
                    break;   // Rank 5
                default:
                    return;
                }
                // remove old enchanting before applying new
                ((Player*)triggerTarget)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,false);
                item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime+1000, 0);
                // add new enchanting
                ((Player*)triggerTarget)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,true);
                return;
            }
            case 812:                               // Periodic Mana Burn
            {
                trigger_spell_id = 25779;           // Mana Burn

                // expected selection current fight target
                triggerTarget = GetTarget()->getVictim();
                if (!triggerTarget || triggerTarget->GetMaxPower(POWER_MANA) <= 0)
                    return;

                triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
                if (!triggeredSpellInfo)
                    return;

                SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
                float max_range = GetSpellMaxRange(srange);
                if (!triggerTarget->IsWithinDist(GetTarget(),max_range))
                    return;

                break;
            }
//                    // Polymorphic Ray
//                    case 6965: break;
//                    // Fire Nova (1-7 ranks)
//                    case 8350:
//                    case 8508:
//                    case 8509:
//                    case 11312:
//                    case 11313:
//                    case 25540:
//                    case 25544:
//                        break;
            case 9712:                              // Thaumaturgy Channel
                trigger_spell_id = 21029;
                break;
//                    // Egan's Blaster
//                    case 17368: break;
//                    // Haunted
//                    case 18347: break;
//                    // Ranshalla Waiting
//                    case 18953: break;
//                    // Inferno
//                    case 19695: break;
//                    // Frostwolf Muzzle DND
//                    case 21794: break;
//                    // Alterac Ram Collar DND
//                    case 21866: break;
//                    // Celebras Waiting
//                    case 21916: break;
			case 23170:                             // Brood Affliction: Bronze
			{
				if (urand(0, 99) < 33)
					target->CastSpell(target, 23171, true, NULL, this);
			    return;
			}
            case 23184:                             // Mark of Frost
            case 25041:                             // Mark of Nature
            {
                std::list<Player*> targets;

                // spells existed in 1.x.x; 23183 - mark of frost; 25042 - mark of nature; both had radius of 100.0 yards in 1.x.x DBC
                // spells are used by Azuregos and the Emerald dragons in order to put a stun debuff on the players which resurrect during the encounter
                // in order to implement the missing spells we need to make a grid search for hostile players and check their auras; if they are marked apply debuff

                // Mark of Frost or Mark of Nature
                uint32 markSpellId = auraId == 23184 ? 23182 : 25040;
                // Aura of Frost or Aura of Nature
                uint32 debufSpellId = auraId == 23184 ? 23186 : 25043;

                MaNGOS::AnyPlayerInObjectRangeWithAuraCheck u_check(GetTarget(), 100.0f, markSpellId);
                MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeWithAuraCheck > checker(targets, u_check);
                Cell::VisitWorldObjects(GetTarget(), checker, 100.0f);

                for (std::list<Player*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    (*itr)->CastSpell((*itr), debufSpellId, true, NULL, NULL, casterGUID);

                return;
            }
            case 23493:                             // Restoration
            {
                int32 heal = triggerTarget->GetMaxHealth() / 10;
                triggerTarget->DealHeal(triggerTarget, heal, auraSpellInfo);

                if (int32 mana = triggerTarget->GetMaxPower(POWER_MANA))
                {
                    mana /= 10;
                    triggerTarget->EnergizeBySpell(triggerTarget, 23493, mana, POWER_MANA);
                }
                return;
            }
//                    // Stoneclaw Totem Passive TEST
//                    case 23792: break;
//                    // Axe Flurry
//                    case 24018: break;
//                    // Mark of Arlokk
//                    case 24210: break;
//                    // Restoration
//                    case 24379: break;
//                    // Happy Pet
//                    case 24716: break;
//                    // Dream Fog
//                    case 24780: break;
//                    // Cannon Prep
//                    case 24832: break;
            case 24834:                             // Shadow Bolt Whirl
            {
                //uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                //uint32 tick = GetAuraTicks();
                //if(tick < 8)
                //{
                //    trigger_spell_id = spellForTick[tick];

                //    // casted in left/right (but triggered spell have wide forward cone)
                //    float forward = target->GetOrientation();
                //    float angle = target->GetOrientation() + ( tick % 2 == 0 ? M_PI_F / 2 : - M_PI_F / 2);
                //    target->SetOrientation(angle);
                //    triggerTarget->CastSpell(triggerTarget, trigger_spell_id, true, NULL, this, casterGUID);
                //    target->SetOrientation(forward);
                //}
                //return;
				uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                uint32 tick = (GetAuraTicks() + 7/*-1*/) % 8;

				// casted in left/right (but triggered spell have wide forward cone)  
				float forward = target->GetOrientation();
                if (tick <= 3)
					{ target->SetOrientation(forward + 0.75f * M_PI_F - tick * M_PI_F / 8); }       // Left
                else
					{ target->SetOrientation(forward - 0.75f * M_PI_F + (8 - tick) * M_PI_F / 8); } // Right
				
				triggerTarget->CastSpell(triggerTarget, spellForTick[tick], true, NULL, this, casterGUID);
				target->SetOrientation(forward);
                return;
            }
            // Stink Trap
            case 24918:
            {
                //10% chance every 5 seconds of proccing a stun
                if (roll_chance_i(10))
                {
                    //Area stun!  Whoo!
                    std::list<Player*> targets;
                    MaNGOS::AnyPlayerInObjectRangeCheck u_check(GetTarget(), 5.0f);
                    MaNGOS::PlayerListSearcher<MaNGOS::AnyPlayerInObjectRangeCheck > checker(targets, u_check);
                    Cell::VisitWorldObjects(GetTarget(), checker, 5.0f);

                    for (std::list<Player*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                        (*itr)->CastSpell((*itr),24919,true,0);
                }
                return;
            }
//                    // Agro Drones
//                    case 25152: break;
            case 25371:                             // Consume
            {
                int32 bpDamage = triggerTarget->GetMaxHealth()*10/100;
                triggerTarget->CastCustomSpell(triggerTarget, 25373, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                return;
            }
//                    // Pain Spike
//                    case 25572: break;
//                    // Rotate 360
//                    case 26009: break;
//                    // Rotate -360
//                    case 26136: break;
//                    // Consume
//                    case 26196: break;
//                    // Berserk
//                    case 26615: break;
//                    // Defile
//                    case 27177: break;
//                    // Teleport: IF/UC
//                    case 27601: break;
//                    // Five Fat Finger Exploding Heart Technique
//                    case 27673: break;
//                    // Nitrous Boost
//                    case 27746: break;
//                    // Steam Tank Passive
//                    case 27747: break;
            case 27808:                             // Frost Blast
            {
                int32 bpDamage = triggerTarget->GetMaxHealth()*26/100;
                triggerTarget->CastCustomSpell(triggerTarget, 29879, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                return;
            }
//                    // Detonate Mana
//                    case 27819: break;
//                    // Controller Timer
//                    case 28095: break;
            // Stalagg Chain and Feugen Chain
            case 28096:
            case 28111:
            {
                // X-Chain is casted by Tesla to X, so: caster == Tesla, target = X
                Unit* pCaster = GetCaster();
                if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT && !pCaster->IsWithinDistInMap(target, 60.0f))
                {
                    pCaster->InterruptNonMeleeSpells(true);
                    ((Creature*)pCaster)->SetInCombatWithZone();
                    // Stalagg Tesla Passive or Feugen Tesla Passive
                    pCaster->CastSpell(pCaster, auraId == 28096 ? 28097 : 28109, true, NULL, NULL, target->GetObjectGuid());
                }
                return;
            }
            // Stalagg Tesla Passive and Feugen Tesla Passive
            case 28097:
            case 28109:
            {
                // X-Tesla-Passive is casted by Tesla on Tesla with original caster X, so: caster = X, target = Tesla
                Unit* pCaster = GetCaster();
                if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT)
                {
                    if (pCaster->getVictim() && !pCaster->IsWithinDistInMap(target, 60.0f))
                    {
                        if (Unit* pTarget = ((Creature*)pCaster)->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                            target->CastSpell(pTarget, 28099, false);// Shock
                    }
                    else
                    {
                        // "Evade"
                        target->RemoveAurasDueToSpell(auraId);
                        target->DeleteThreatList();
                        target->CombatStop(true);
                        // Recast chain (Stalagg Chain or Feugen Chain
                        target->CastSpell(pCaster, auraId == 28097 ? 28096 : 28111, false);
                    }
                }
                return;
            }
//                    // Mark of Didier
//                    case 28114: break;
//                    // Communique Timer, camp
//                    case 28346: break;
//                    // Icebolt
//                    case 28522: break;
//                    // Silithyst
//                    case 29519: break;
//                    // Guardian of Icecrown Passive
//                    case 29897: break;
//                    // Mind Exhaustion Passive
//                    case 30025: break;
//                    // Regeneration
//                    case 30799:
//                    case 30800:
//                    case 30801:
//                        break;
            default:
                break;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch(auraId)
            {
            case 66:                                // Invisibility
                // Here need periodic trigger reducing threat spell (or do it manually)
                return;
            default:
                break;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            switch(auraId)
            {
                // Wild Magic - Mage debuff at Nefarian in BWL.
            case 23410:
                triggerTarget->InterruptNonMeleeSpells(false);
                triggerTarget->CastSpell(triggerTarget, 23603, true);
                return;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch(auraId)
            {
            case 768:                               // Cat Form
                // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                return;
            case 22842:                             // Frenzied Regeneration
            case 22895:
            case 22896:
            case 26999:
            {
                int32 LifePerRage = GetModifier()->m_amount;

                int32 lRage = target->GetPower(POWER_RAGE);
                if (lRage > 100)                    // rage stored as rage*10
                    lRage = 100;
                target->ModifyPower(POWER_RAGE, -lRage);
                int32 FRTriggerBasePoints = int32(lRage*LifePerRage/10);
                target->CastCustomSpell(target, 22845, &FRTriggerBasePoints, NULL, NULL, true, NULL, this);
                return;
            }
            default:
                break;
            }
            break;
        }

//            case SPELLFAMILY_HUNTER:
//            {
//                switch(auraId)
//                {
//                    //Frost Trap Aura
//                    case 13810:
//                        return;
//                    // Tame spells
//                    case 19597:         // Tame Ice Claw Bear
//                    case 19676:         // Tame Snow Leopard
//                    case 19677:         // Tame Large Crag Boar
//                    case 19678:         // Tame Adult Plainstrider
//                    case 19679:         // Tame Prairie Stalker
//                    case 19680:         // Tame Swoop
//                    case 19681:         // Tame Dire Mottled Boar
//                    case 19682:         // Tame Surf Crawler
//                    case 19683:         // Tame Armored Scorpid
//                    case 19684:         // Tame Webwood Lurker
//                    case 19685:         // Tame Nightsaber Stalker
//                    case 19686:         // Tame Strigid Screecher
//                        return;
//                    default:
//                        break;
//                }
//                break;
//            }
        case SPELLFAMILY_SHAMAN:
        {
            switch(auraId)
            {
            case 28820:                             // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
            {
                // Need remove self if Lightning Shield not active
                Unit::SpellAuraHolderMap const& auras = triggerTarget->GetSpellAuraHolderMap();
                for(Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    SpellEntry const* spell = itr->second->GetSpellProto();
                    if (spell->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                            (spell->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                        return;
                }
                triggerTarget->RemoveAurasDueToSpell(28820);
                return;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        // Reget trigger spell proto
        triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    }
    else
    {
        // Spell exist but require custom code
        switch(auraId)
        {
        case 9347:                                      // Mortal Strike
        {
            // expected selection current fight target
            triggerTarget = GetTarget()->getVictim();
            if (!triggerTarget)
                return;

            // avoid triggering for far target
            SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
            float max_range = GetSpellMaxRange(srange);
            if (!triggerTarget->IsWithinDist(GetTarget(),max_range))
                return;

            break;
        }
        case 1010:                                      // Curse of Idiocy
        {
            // TODO: spell casted by result in correct way mostly
            // BUT:
            // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
            //      but must show affect apply like item casting
            // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

            // prevent cast by triggered auras
            if (casterGUID == triggerTarget->GetObjectGuid())
                return;

            // stop triggering after each affected stats lost > 90
            int32 intelectLoss = 0;
            int32 spiritLoss = 0;

            Unit::AuraList const& mModStat = triggerTarget->GetAurasByType(SPELL_AURA_MOD_STAT);
            for(Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
            {
                if ((*i)->GetId() == 1010)
                {
                    switch((*i)->GetModifier()->m_miscvalue)
                    {
                    case STAT_INTELLECT:
                        intelectLoss += (*i)->GetModifier()->m_amount;
                        break;
                    case STAT_SPIRIT:
                        spiritLoss   += (*i)->GetModifier()->m_amount;
                        break;
                    default:
                        break;
                    }
                }
            }

            if (intelectLoss <= -90 && spiritLoss <= -90)
                return;

            break;
        }
        case 13278:                                     // Gnomish Death Ray
        {
            Unit* caster = GetCaster();

            caster->CastSpell(caster, 13280, true);
            return;
        }
        case 16191:                                     // Mana Tide
        {
            triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
            return;
        }
        case 19636:										//Fire Blossom
        {
            Unit* caster = GetCaster();
            if (caster && caster->GetTypeId() == TYPEID_UNIT)
            {
                Creature* unitCaster = (Creature*)caster;
                if (unitCaster && unitCaster->CanHaveThreatList())
                {
                    ThreatList tList = unitCaster->getThreatManager().getThreatList();
                    std::vector<uint32> L;
                    for (uint32 i = 0; i < tList.size(); i++)
                        L.push_back(i);
                    while (!L.empty())
                    {
                        uint32 rnd = urand(0, L.size()-1);
                        Unit* spellTarget = unitCaster->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, L[rnd]);
                        if (spellTarget && spellTarget->isAlive() && caster->GetDistance(spellTarget) < 40.0f)
                        {
                            unitCaster->CastSpell(spellTarget, triggeredSpellInfo, true, NULL, this, caster->GetGUID());
                            break;
                        }
                        L.erase(L.begin()+rnd);
                    }
                }
            }
            return;
        }
        case 19695:                                     //Baron Geddon's Inferno spell
        {
            int32 dmg = 1000 + 500*(GetAuraTicks()/3);
            if (Unit* caster = GetCaster())
                caster->CastCustomSpell(caster, 19698, &dmg, NULL, NULL, true, NULL, this, caster->GetGUID());
            return;
        }
        case 21926: // The Giantstalker's 5/8-bonus; Nature's Ally.
        {
            Pet* pet = target->GetPet();
            if (pet)
            {
                if (pet->HasAura(21925))
                    pet->SetGiantstalkerBuffApplied(true);
                else
                    pet->SetGiantstalkerBuffApplied(false);

                SpellAuraHolder *holder = CreateSpellAuraHolder(triggeredSpellInfo, true, pet, target);

                for(uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    uint8 eff = triggeredSpellInfo->Effect[i];

                    if (eff >= TOTAL_SPELL_EFFECTS)
                        continue;

                    Aura *aur = CreateAura(triggeredSpellInfo, SpellEffectIndex(i), NULL, holder, pet);
                    holder->AddAura(aur, SpellEffectIndex(i));
                }
                pet->AddSpellAuraHolder(holder);
            }

            return;
        }
        case 23620:										// Vaelastrasz' Burning Adrenaline spell.
        {
            if (GetAuraDuration() < 1000)
            {
                triggerTarget->CastSpell(triggerTarget, 23478, true);	// Cause the player to explode.
                triggerTarget->CastSpell(triggerTarget, 23644, true);   // Kill the player.
                return;
            }
            break;
        }

        }
    }

    Unit* triggerCaster = triggerTarget;
    WorldObject* triggerTargetObject = NULL;

    // for channeled spell cast applied from aura owner to channel target (persistent aura affects already applied to true target)
    // come periodic casts applied to targets, so need seelct proper caster (ex. 15790)
    if (IsChanneledSpell(GetSpellProto()) && GetSpellProto()->Effect[GetEffIndex()] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
    {
        // interesting 2 cases: periodic aura at caster of channeled spell
        if (target->GetObjectGuid() == casterGUID)
        {
            triggerCaster = target;

            if (WorldObject* channelTarget = target->GetMap()->GetWorldObject(target->GetChannelObjectGuid()))
            {
                if (channelTarget->isType(TYPEMASK_UNIT))
                    triggerTarget = (Unit*)channelTarget;
                else
                    triggerTargetObject = channelTarget;
            }
        }
        // or periodic aura at caster channel target
        else if (Unit* caster = GetCaster())
        {
            if (target->GetObjectGuid() == caster->GetChannelObjectGuid())
            {
                triggerCaster = caster;
                triggerTarget = target;
            }
        }
    }

    // All ok cast by default case
    if (triggeredSpellInfo)
    {
        if (triggerTargetObject)
            triggerCaster->CastSpell(triggerTargetObject->GetPositionX(), triggerTargetObject->GetPositionY(), triggerTargetObject->GetPositionZ(),
                                     triggeredSpellInfo, true, NULL, this, casterGUID);
        else
            triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, NULL, this, casterGUID);
    }
    else
    {
        if (Unit* caster = GetCaster())
        {
            if (triggerTarget->GetTypeId() != TYPEID_UNIT || !sScriptMgr.OnEffectDummy(caster, GetId(), GetEffIndex(), (Creature*)triggerTarget))
                sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?",GetId(),GetEffIndex());
        }
    }
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // AT APPLY
    if (apply)
    {
        switch(GetSpellProto()->SpellFamilyName)
        {
        case SPELLFAMILY_GENERIC:
        {
            switch(GetId())
            {
            case 126:  // Eye of Kilrogg
            {
                // For the Eye of Kilrogg
                Player* player = dynamic_cast<Player*>(GetCaster());

                if (player)
                {
                    Pet* eye_of_kilrogg = player->FindGuardianWithEntry(4277);
                    if (eye_of_kilrogg)
                    {
                        player->GetCamera().SetView(eye_of_kilrogg);
                        player->SetClientControl(eye_of_kilrogg, 1);
                        player->SetMover(eye_of_kilrogg);
                    }
                }

                break;
            }

            case 7057:                              // Haunting Spirits
                // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                m_isPeriodic = true;
                m_modifier.periodictime = 30*IN_MILLISECONDS;
                m_periodicTimer = m_modifier.periodictime;
                return;
			case 16468:                              // Mother's Milk
                // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                m_isPeriodic = true;
                m_modifier.periodictime = 30*IN_MILLISECONDS;
                m_periodicTimer = m_modifier.periodictime;
                return;
			case 24596:								// Intoxicating Venom
				// expected to tick with 15 sec period (tick part see in Aura::PeriodicTick)
                m_isPeriodic = true;
                m_modifier.periodictime = 15*IN_MILLISECONDS;
                m_periodicTimer = m_modifier.periodictime;
				return;
            case 12292:								// Warriors' sweeping strikes should not be removed on stance change.
            {
                SpellAuraHolder* holder = GetTarget()->GetSpellAuraHolder(GetId());
                if (holder)
                    holder->SetAuraLostAtShapeShift(false);
            }
            case 10255:                             // Stoned
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetTypeId() != TYPEID_UNIT)
                        return;

                    //caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    caster->addUnitState(UNIT_STAT_ROOT);
                }
                return;
            }
            // Electromagnetic Gigaflux Reactivator
            case 11826:
            {
                const SpellEntry *spellInfo = sSpellStore.LookupEntry(29151);

                Spell *tmp_spell = new Spell(GetCaster(), spellInfo, true);

                Spell::UnitList targetUnitMap;
                tmp_spell->FillAreaTargets(targetUnitMap, 20.f, PUSH_IN_FRONT_LARGE, SPELL_TARGETS_HOSTILE);

                for (Unit* target : targetUnitMap)
                {
                    Spell *spell = new Spell(GetCaster(), spellInfo, true);
                    SpellCastTargets targets;
                    targets.setUnitTarget(target);
                    spell->prepare(&targets, this);
                }

                delete tmp_spell;
                return;
            }
            case 13139:                             // net-o-matic
                // root to self part of (root_target->charge->root_self sequence
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, 13138, true, NULL, this);
                return;
            case 23183:								// Aura of Frost
            {
                if (apply)
                {
                    Unit* target = GetTarget();
                    if (target && target->isAlive() && target->HasAura(23182) && !target->HasAura(23186))
                        target->CastSpell(target, 23186, true);
                }
                return;
            }
            case 25042:								// Aura of Nature
            {
                if (apply)
                {
                    Unit* target = GetTarget();
                    if (target && target->isAlive() && target->HasAura(25040) && !target->HasAura(25043))
                        target->CastSpell(target, 25043, true);
                }
                return;
            }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Earth Shield
            if ((GetSpellProto()->SpellFamilyFlags & UI64LIT(0x40000000000)))
            {
                // prevent double apply bonuses
                if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
                {
                    if (Unit* caster = GetCaster())
                    {
                        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                        m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                    }
                }
                return;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            switch (GetId())
            {
            case 23427:	// The Warlock specific spell at Nefarian.
                target->CastSpell(target, 23426, true);
                target->CastSpell(target, 23426, true);
                return;
            }

            break;
        }

        }
    }
    // AT REMOVE
    else
    {
        if (IsQuestTameSpell(GetId()) && target->isAlive())
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->isAlive())
                return;

            uint32 finalSpellId = 0;
            switch(GetId())
            {
            case 19548:
                finalSpellId = 19597;
                break;
            case 19674:
                finalSpellId = 19677;
                break;
            case 19687:
                finalSpellId = 19676;
                break;
            case 19688:
                finalSpellId = 19678;
                break;
            case 19689:
                finalSpellId = 19679;
                break;
            case 19692:
                finalSpellId = 19680;
                break;
            case 19693:
                finalSpellId = 19684;
                break;
            case 19694:
                finalSpellId = 19681;
                break;
            case 19696:
                finalSpellId = 19682;
                break;
            case 19697:
                finalSpellId = 19683;
                break;
            case 19699:
                finalSpellId = 19685;
                break;
            case 19700:
                finalSpellId = 19686;
                break;
            case 30646:
                finalSpellId = 30647;
                break;
            case 30653:
                finalSpellId = 30648;
                break;
            case 30654:
                finalSpellId = 30652;
                break;
            case 30099:
                finalSpellId = 30100;
                break;
            case 30102:
                finalSpellId = 30103;
                break;
            case 30105:
                finalSpellId = 30104;
                break;
            }

            if (finalSpellId)
                caster->CastSpell(target, finalSpellId, true, NULL, this);

            return;
        }

        switch(GetId())
        {
        case 126:  // Eye of Kilrogg
        {
            // For the Eye of Kilrogg
            Player* player = dynamic_cast<Player*>(GetCaster());

            if (player)
            {
		  player->GetCamera().SetView(player);
		  player->SetClientControl(player, 1);
		  player->SetMover(player);
		  
		  Pet* eye_of_kilrogg = player->FindGuardianWithEntry(4277);
		  if (eye_of_kilrogg)
		  {
		      player->RemoveGuardian(eye_of_kilrogg);
		      eye_of_kilrogg->DealDamage(eye_of_kilrogg, eye_of_kilrogg->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
		  }
            }
            break;
        }
        case 10255:                                     // Stoned
        {
            if (Unit* caster = GetCaster())
            {
                if (caster->GetTypeId() != TYPEID_UNIT)
                    return;

                // see dummy effect of spell 10254 for removal of flags etc
                caster->CastSpell(caster, 10254, true);
            }
            return;
        }
        case 12479:                                     // Hex of Jammal'an
            target->CastSpell(target, 12480, true, NULL, this);
            return;
        case 12774:                                     // (DND) Belnistrasz Idol Shutdown Visual
        {
            if (m_removeMode == AURA_REMOVE_BY_DEATH)
                return;

            // Idom Rool Camera Shake <- wtf, don't drink while making spellnames?
            if (Unit* caster = GetCaster())
                caster->CastSpell(caster, 12816, true);

            return;
        }
        case 28169:                                     // Mutating Injection
        {
            // Mutagen Explosion
            target->CastSpell(target, 28206, true, NULL, this);
            // Poison Cloud
            target->CastSpell(target, 28240, true, NULL, this);
            return;
        }
        }

        if (m_removeMode == AURA_REMOVE_BY_DEATH)
        {
            // Stop caster Arcane Missle chanelling on death
            if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_MAGE && (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000800)))
            {
                if (Unit* caster = GetCaster())
                    caster->InterruptSpell(CURRENT_CHANNELED_SPELL);

                return;
            }
        }
    }

    // AT APPLY & REMOVE

    switch(GetSpellProto()->SpellFamilyName)
    {
    case SPELLFAMILY_GENERIC:
    {
        switch(GetId())
        {
        case 6606:                                  // Self Visual - Sleep Until Cancelled (DND)
        {
            if (apply)
            {
                target->SetStandState(UNIT_STAND_STATE_SLEEP);
                target->addUnitState(UNIT_STAT_ROOT);
            }
            else
            {
                target->clearUnitState(UNIT_STAT_ROOT);
                target->SetStandState(UNIT_STAND_STATE_STAND);
            }

            return;
        }
        case 24658:                                 // Unstable Power
        {
            if (apply)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                caster->CastSpell(target, 24659, true, NULL, NULL, GetCasterGuid());
            }
            else
                target->RemoveAurasDueToSpell(24659);
            return;
        }
        case 24661:                                 // Restless Strength
        {
            if (apply)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                caster->CastSpell(target, 24662, true, NULL, NULL, GetCasterGuid());
            }
            else
                target->RemoveAurasDueToSpell(24662);
            return;
        }
        case 29266:                                 // Permanent Feign Death
        {
            // Unclear what the difference really is between them.
            // Some has effect1 that makes the difference, however not all.
            // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
            // For now, just handle all the same way
            if (target->GetTypeId() == TYPEID_UNIT)
                target->SetFeignDeath(apply);

            return;
        }
        case 27978:
            if (apply)
                target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
            else
                target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
            return;
        }
        break;
    }
    case SPELLFAMILY_DRUID:
    {
        // Predatory Strikes
        if (target->GetTypeId()==TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
        {
            ((Player*)target)->UpdateAttackPowerAndDamage();
            return;
        }
        break;
    }
    case SPELLFAMILY_ROGUE:
        break;
    case SPELLFAMILY_HUNTER:
        break;
    case SPELLFAMILY_PALADIN:
    {
        if (GetSpellProto()->SpellIconID == 237 && GetSpellProto()->SpellVisual == 5622) {    //Paladin Seal of the crusader
            if (apply)
            {
                m_modifier.m_amount = (-1)*m_spellAuraHolder->GetAuraByEffectIndex(EFFECT_INDEX_1)->GetModifier()->m_amount;
                m_modifier.m_miscvalue = SPELL_SCHOOL_MASK_NORMAL;
                m_modifier.periodictime = 0;
            }
            HandleModDamagePercentDone(apply,true);
            return;
        }

        // For the Paladins' Libram of Divinity.
        if (GetSpellProto()->Id == 28853)
        {
            SpellEntry const* spell = sSpellStore.LookupEntry(19750);

            Player* player_target = dynamic_cast<Player*>(GetTarget());
            if (player_target)
            {
                if (apply)
                {
                    SpellModifier *healmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_FLAT, 53, 19750, spell->SpellFamilyFlags);
                    if (healmod)
                        player_target->AddSpellMod(healmod, true);
                }
                else
                {
                    SpellModifier *current_healmod = player_target->GetSpellMod(SPELLMOD_EFFECT1, 19750);
                    if (current_healmod)
                        player_target->AddSpellMod(current_healmod, false);
                }
            }
            return;
        }

        break;
    }
    case SPELLFAMILY_SHAMAN:
    {
        switch(GetId())
        {
        case 6495:                                  // Sentry Totem
        {
            if (target->GetTypeId() != TYPEID_PLAYER)
                return;

            Totem* totem = target->GetTotem(TOTEM_SLOT_AIR);

            if (totem && apply)
                ((Player*)target)->GetCamera().SetView(totem);
            else
                ((Player*)target)->GetCamera().ResetView();

            return;
        }
        }
        // Improved Weapon Totems
        if (GetSpellProto()->SpellIconID == 57 && target->GetTypeId()==TYPEID_PLAYER)
        {
            if (apply)
            {
                switch (m_effIndex)
                {
                case 0:
                    // Windfury Totem
                    m_spellmod = new SpellModifier(SPELLMOD_EFFECT1,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x00200000000));
                    break;
                case 1:
                    // Flametongue Totem
                    m_spellmod = new SpellModifier(SPELLMOD_EFFECT1,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x00400000000));
                    break;
                default:
                    return;
                }
            }

            ((Player*)target)->AddSpellMod(m_spellmod, apply);
            return;
        }
        break;
    }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId()))
    {
        if (apply)
            target->AddPetAura(petSpell);
        else
            target->RemovePetAura(petSpell);
        return;
    }

    if (GetEffIndex() == EFFECT_INDEX_0 && target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if (!itr->second->IsFitToRequirements((Player*)target, zone, area))
                    target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if (itr->second->autocast)
                {
                    if (!target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                        target->CastSpell(target, itr->second->spellId, true);
                }
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (target->GetTypeId() == TYPEID_UNIT)
        sScriptMgr.OnAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if(!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 display_id = Creature::ChooseDisplayId(ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        target->Mount(display_id, GetId());
    }
    else
    {
        target->Unmount(true);
    }
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_WATER_WALK, 8+4);
    else
        data.Initialize(SMSG_MOVE_LAND_WALK, 8+4);
    data << GetTarget()->GetPackGUID();
    data << uint32(0);
    GetTarget()->SendMessageToSet(&data, true);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;
    Unit *target = GetTarget();
    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8+4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8+4);
    data << target->GetPackGUID();
    data << uint32(0);
    target->SendMessageToSet(&data, true);

    // start fall from current height
    if(!apply && target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->SetFallInformation(0, target->GetPositionZ());
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    GetTarget()->SendHover(apply);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if(GetTarget()->GetTypeId()==TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    if(!Real)
        return;

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

    Unit *target = GetTarget();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (!ssEntry)
    {
        sLog.outError("Unknown shapeshift form %u in spell %u", form, GetId());
        return;
    }

    switch(form)
    {
    case FORM_CAT:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 892;
        else
            modelid = 8571;
        PowerType = POWER_ENERGY;
        break;
    case FORM_TRAVEL:
        modelid = 632;
        break;
    case FORM_AQUA:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 2428;
        else
            modelid = 2428;
        break;
    case FORM_BEAR:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 2281;
        else
            modelid = 2289;
        PowerType = POWER_RAGE;
        break;
    case FORM_GHOUL:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 10045;
        break;
    case FORM_DIREBEAR:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 2281;
        else
            modelid = 2289;
        PowerType = POWER_RAGE;
        break;
    case FORM_CREATUREBEAR:
        modelid = 902;
        break;
    case FORM_GHOSTWOLF:
        modelid = 4613;
        break;
    case FORM_MOONKIN:
        if(Player::TeamForRace(target->getRace()) == ALLIANCE)
            modelid = 15374;
        else
            modelid = 15375;
        break;
    case FORM_AMBIENT:
    case FORM_SHADOW:
    case FORM_STEALTH:
        break;
    case FORM_TREE:
        modelid = 864;
        break;
    case FORM_BATTLESTANCE:
    case FORM_BERSERKERSTANCE:
    case FORM_DEFENSIVESTANCE:
        PowerType = POWER_RAGE;
        break;
    case FORM_SPIRITOFREDEMPTION:
        modelid = 16031;
        break;
    default:
        break;
    }

    // remove polymorph before changing display id to keep new display id
    switch (form)
    {
    case FORM_CAT:
    case FORM_TREE:
    case FORM_TRAVEL:
    case FORM_AQUA:
    case FORM_BEAR:
    case FORM_DIREBEAR:
    case FORM_MOONKIN:
    {
        //set normal Tauren Scale and Nightelf scale for travel shapeshift
        if (target->getRace() == RACE_TAUREN)
        {
            if (target->getGender() == GENDER_MALE)
                target->SetObjectScale(1.35f);
            else
                target->SetObjectScale(1.25f);
        }
        else
            target->SetObjectScale(DEFAULT_OBJECT_SCALE);

        // remove movement affects
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT, GetHolder());
        Unit::AuraList const& slowingAuras = target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
        for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
        {
            SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

            uint32 aurMechMask = GetAllSpellMechanicMask(aurSpellInfo);

            // If spell that caused this aura has Croud Control or Daze effect
            if ((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
                    // some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
                    (aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
                     (aurMechMask & (1 << (MECHANIC_SNARE-1))) == 0))
            {
                ++iter;
                continue;
            }

            // All OK, remove aura now
            target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
            iter = slowingAuras.begin();
        }

        // and polymorphic affects
        if (target->IsPolymorphed())
            target->RemoveAurasDueToSpell(target->getTransForm());

        break;
    }
    default:
        break;
    }

    if (apply)
    {
        // remove other shapeshift before applying a new one
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT, GetHolder());

        // need send to client not form active state, or at re-apply form client go crazy
        // target->SendForcedObjectUpdate();                -- not need in pre-3.x

        if (modelid > 0)
        {
            target->SetDisplayId(modelid);

            //Tauren shapeshift get a small scale
            if (target->getRace() == RACE_TAUREN)
            {
                switch(modelid)
                {
                case 8571:
                case 864:
                case 2428:
                case 2289:
                case 15375:
                    target->SetObjectScale(DEFAULT_OBJECT_SCALE);
                    break;
                }
            }
            //Travel Scale for NE and Tauren
            if (modelid == 632)
                target->SetObjectScale(0.8f);
        }

        if (PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            if (target->getPowerType() != PowerType)
                target->setPowerType(PowerType);

            switch (form)
            {
            case FORM_CAT:
            case FORM_BEAR:
            case FORM_DIREBEAR:
            {
                // get furor proc chance
                int32 furorChance = 0;
                Unit::AuraList const& mDummy = target->GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 238)
                    {
                        furorChance = (*i)->GetModifier()->m_amount;
                        break;
                    }
                }

                if (m_modifier.m_miscvalue == FORM_CAT)
                {
                    target->SetPower(POWER_ENERGY, 0);
                    if (irand(1,100) <= furorChance)
                        target->CastSpell(target, 17099, true, NULL, this);
                }
                else
                {
                    target->SetPower(POWER_RAGE, 0);
                    if(irand(1,100) <= furorChance)
                        target->CastSpell(target, 17057, true, NULL, this);
                }
                break;
            }
            case FORM_BATTLESTANCE:
            case FORM_DEFENSIVESTANCE:
            case FORM_BERSERKERSTANCE:
            {
                uint32 Rage_val = 0;
                //Tactical mastery
                if(target->GetTypeId() == TYPEID_PLAYER)
                {
                    Unit::AuraList const& aurasOverrideClassScripts = target->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                    for(Unit::AuraList::const_iterator iter = aurasOverrideClassScripts.begin(); iter != aurasOverrideClassScripts.end(); ++iter)
                    {
                        // select by script id
                        switch((*iter)->GetModifier()->m_miscvalue)
                        {
                        case 831:
                            Rage_val =  50;
                            break;
                        case 832:
                            Rage_val = 100;
                            break;
                        case 833:
                            Rage_val = 150;
                            break;
                        case 834:
                            Rage_val = 200;
                            break;
                        case 835:
                            Rage_val = 250;
                            break;
                        }
                        if(Rage_val!=0)
                            break;
                    }
                }
                if (target->GetPower(POWER_RAGE) > Rage_val)
                    target->SetPower(POWER_RAGE, Rage_val);
                break;
            }
            default:
                break;
            }
        }

        target->SetShapeshiftForm(form);
    }
    else
    {
        if(modelid > 0)
            target->SetDisplayId(target->GetNativeDisplayId());
        if(target->getClass() == CLASS_DRUID)
            target->setPowerType(POWER_MANA);
        target->SetShapeshiftForm(FORM_NONE);
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);

    if(target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // update active transform spell only not set or not overwriting negative by positive case
    if (apply && (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm())))
    {
        target->setTransForm(GetId());

        // special case (spell specific functionality)
        if (m_modifier.m_miscvalue == 0)
        {
            switch (GetId())
            {
            case 16739:                                 // Orb of Deception
            {
                uint32 orb_model = target->GetNativeDisplayId();
                switch(orb_model)
                {
                    // Troll Female
                case 1479:
                    target->SetDisplayId(10134);
                    break;
                    // Troll Male
                case 1478:
                    target->SetDisplayId(10135);
                    break;
                    // Tauren Male
                case 59:
                    target->SetDisplayId(10136);
                    break;
                    // Human Male
                case 49:
                    target->SetDisplayId(10137);
                    break;
                    // Human Female
                case 50:
                    target->SetDisplayId(10138);
                    break;
                    // Orc Male
                case 51:
                    target->SetDisplayId(10139);
                    break;
                    // Orc Female
                case 52:
                    target->SetDisplayId(10140);
                    break;
                    // Dwarf Male
                case 53:
                    target->SetDisplayId(10141);
                    break;
                    // Dwarf Female
                case 54:
                    target->SetDisplayId(10142);
                    break;
                    // NightElf Male
                case 55:
                    target->SetDisplayId(10143);
                    break;
                    // NightElf Female
                case 56:
                    target->SetDisplayId(10144);
                    break;
                    // Undead Female
                case 58:
                    target->SetDisplayId(10145);
                    break;
                    // Undead Male
                case 57:
                    target->SetDisplayId(10146);
                    break;
                    // Tauren Female
                case 60:
                    target->SetDisplayId(10147);
                    break;
                    // Gnome Male
                case 1563:
                    target->SetDisplayId(10148);
                    break;
                    // Gnome Female
                case 1564:
                    target->SetDisplayId(10149);
                    break;
                default:
                    break;
                }
                break;
            }
            default:
                sLog.outError("Aura::HandleAuraTransform, spell %u does not have creature entry defined, need custom defined model.", GetId());
                break;
            }
        }
        else
        {
            uint32 model_id;
            int32 miscValue = m_modifier.m_miscvalue;

            if (miscValue == 15219) //Random halloween critter
            {
                switch(urand(0,9))
                {
                case 0:
                    miscValue = 7551;
                    break;	//Frog
                case 1:
                    miscValue = 14866;
                    break;	//Frog
                case 2:
                case 3:
                    miscValue = 7385;
                    break;	//Cat
                case 4:
                    miscValue = 10925;
                    break; //Snake
                case 5:
                    miscValue = 11320;
                    break; //Snake
                case 6:
                    miscValue = 11326;
                    break; //Mini-diablo
                case 7:
                case 8:
                    miscValue = 1512;
                    break; //Bat
                case 9:
                    miscValue = 12381;
                    break; //Wisp
                default:
                    break;
                }
            }

            CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(miscValue);
            if (!ci)
            {
                model_id = 16358;                           // pig pink ^_^
                sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", miscValue, GetId());
            }
            else
            {
                model_id = Creature::ChooseDisplayId(ci);   // Will use the default model here

                if (GetSpellProto() && GetSpellProto()->Id == 28271) // The different colours for the turtle polymorph.
                {
                    int chance = urand(0, 99);

                    if (chance < 33)
                        model_id = 16360;
                    else if (chance < 66)
                        model_id = 16361;
                }

                if (GetSpellProto() && GetSpellProto()->Id == 28271) // For setting the correct scale for the turtle polymorph.
                    target->SetObjectScale(0.75);
                else if (ci->scale)
                    target->SetObjectScale(ci->scale);
                else
                    target->SetObjectScale(1);

                // Choose a model for the Wild Polymorph-spell.
                // Don't know if these all are actual things you could get polymorphed into, but there's nothing wrong with some fun, right? :)
                if (GetId() == 23603)
                {
                    switch(urand(0, 10))
                    {
                    case 0:
                        break;						// Carrion Grub
                    case 1:
                        model_id = 328;
                        break;		// Rabbit
                    case 2:
                        model_id = 4473;
                        break;		// Giraffe
                    case 3:
                        model_id = 134;
                        break;		// Squirrel
                    case 4:
                        model_id = 1060;
                        break;		// Cow
                    case 5:
                        model_id = 1206;
                        break;		// Adder
                    case 6:
                        model_id = 6087;
                        break;		// Zhevra
                    case 7:
                        model_id = 1141;
                        break;		// Rat
                    case 8:
                        model_id = 1088;
                        break;		// Spider
                    case 9:
                        model_id = 1220;
                        break;		// Plainstrider
                    case 10:
                        model_id = 5555;
                        break;	// Cat
                    }

                }

                //Reapply scale modifiers
                Unit::AuraList const& scaleTransforms = target->GetAurasByType(SPELL_AURA_MOD_SCALE);
                if (!scaleTransforms.empty())
                {
                    // look for other transform auras
                    for(Unit::AuraList::const_iterator i = scaleTransforms.begin(); i != scaleTransforms.end(); ++i)
                    {
                        (*i)->ApplyModifier(true);
                    }
                }
            }

            //If the object we're transforming to is gendered & we're gendered, then there's a 50/50 chance that ChooseDisplayId returned us
            //the wrong gender, so we need to change the model ID to the right one
            CreatureModelInfo const* model_info = sObjectMgr.GetCreatureModelInfo(model_id);
            uint8 targetGender = target->getGender();
            if (targetGender != GENDER_NONE && targetGender != model_info->gender && model_info->gender != GENDER_NONE && model_info->modelid_other_gender != 0)
                model_id = model_info->modelid_other_gender;

            target->SetDisplayId(model_id);

            // creature case, need to update equipment
            if (ci && target->GetTypeId() == TYPEID_UNIT)
                ((Creature*)target)->LoadEquipment(ci->equipmentId, true);
        }
    }
    else if (!apply && target->getTransForm() == GetId())
    {
        // ApplyModifier(true) will reapply it if need
        target->setTransForm(0);
        target->SetDisplayId(target->GetNativeDisplayId());

        Creature *creature = dynamic_cast<Creature*>(target);

        if (creature)
        {
            target->SetObjectScale(creature->GetCreatureInfo()->scale);
        } else
        {
            if (target->getRace() == RACE_TAUREN)
            {
                if (target->getGender() == GENDER_MALE)
                    target->SetObjectScale(1.35f);
                else
                    target->SetObjectScale(1.25f);
            } else
                target->SetObjectScale(1);
        }

        //Reapply scale modifiers
        Unit::AuraList const& scaleTransforms = target->GetAurasByType(SPELL_AURA_MOD_SCALE);
        if (!scaleTransforms.empty())
        {
            // look for other transform auras
            for(Unit::AuraList::const_iterator i = scaleTransforms.begin(); i != scaleTransforms.end(); ++i)
            {
                (*i)->ApplyModifier(true);
            }
        }

        // apply default equipment for creature case
        if (target->GetTypeId() == TYPEID_UNIT)
            ((Creature*)target)->LoadEquipment(((Creature*)target)->GetCreatureInfo()->equipmentId, true);

        // re-apply some from still active with preference negative cases
        Unit::AuraList const& otherTransforms = target->GetAurasByType(SPELL_AURA_TRANSFORM);
        if (!otherTransforms.empty())
        {
            // look for other transform auras
            Aura* handledAura = *otherTransforms.begin();
            for(Unit::AuraList::const_iterator i = otherTransforms.begin(); i != otherTransforms.end(); ++i)
            {
                // negative auras are preferred
                if (!IsPositiveSpell((*i)->GetSpellProto()->Id))
                {
                    handledAura = *i;
                    break;
                }
            }
            handledAura->ApplyModifier(true);
        }
    }

    // Make sure you cannot do anything while affected by the wild polymorph. (Nefarian's mage class call.)
    if ( GetId() == 23603 )
        target->SetConfused(apply, GetCasterGuid(), GetId());

    // The Discombobulator Ray should dismount the player.
    if (apply && GetId() == 4060)
    {
        Player* player = dynamic_cast<Player*>(target);
        if (player)
        {
            player->Unmount(true);
            player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        }

    }



}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(!Real)
        return;

    Player* player = (Player*)GetTarget();

    uint32 faction_id = m_modifier.m_miscvalue;
    ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
        player->StopAttackFaction(faction_id);
}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // For the Zandalarian Hero Badge.
    if (GetSpellProto()->Id == 24575 && GetAuraDuration() / GetAuraMaxDuration() >= 1.f)
    {
        if (m_modifier.m_amount == 3)
            m_modifier.m_amount = 30;
    }

    uint32 prot=GetSpellProto()->EffectMiscValue[m_effIndex];
    int32 points = GetModifier()->m_amount;

    ((Player*)GetTarget())->ModifySkillBonus(prot, (apply ? points: -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
    if(prot == SKILL_DEFENSE)
        ((Player*)GetTarget())->UpdateDefenseBonusesMod();
}

void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
    if (Real && !apply)
    {
        if (m_removeMode != AURA_REMOVE_BY_DEATH)
            return;
        // Item amount
        if (m_modifier.m_amount <= 0)
            return;

        SpellEntry const *spellInfo = GetSpellProto();
        if (spellInfo->EffectItemType[m_effIndex] == 0)
            return;

        Unit* victim = GetTarget();
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        // Soul Shard (target req.)
        if (spellInfo->EffectItemType[m_effIndex] == 6265)
        {
            // Only from non-grey units
            if (!((Player*)caster)->isHonorOrXPTarget(victim) ||
                    (victim->GetTypeId() == TYPEID_UNIT && !((Player*)caster)->isAllowedToLoot((Creature*)victim)))
                return;
        }

        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_modifier.m_amount;

        ItemPosCountVec dest;
        InventoryResult msg = ((Player*)caster)->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
        if( msg != EQUIP_ERR_OK )
        {
            count-=noSpaceForCount;
            ((Player*)caster)->SendEquipError( msg, NULL, NULL, spellInfo->EffectItemType[m_effIndex] );
            if (count==0)
                return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, count, true, true);
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue-1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue-1));
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue-1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue-1));
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
    GetTarget()->UpdateModelData();
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    // not charm yourself
    if (GetCasterGuid() == target->GetObjectGuid())
        return;

    Player* caster = dynamic_cast<Player*>(GetCaster());
    if(!caster)
        return;

    Camera& player_camera = caster->GetCamera();

    if( apply )
    {
        // is it really need after spell check checks?
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM, GetHolder());
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS, GetHolder());

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        target->SetCharmerGuid(GetCasterGuid());
        target->setFaction(caster->getFaction());
        target->CastStop(target == caster ? GetId() : 0);
        caster->SetCharm(target);

        player_camera.SetView(target);
        caster->SetClientControl(target, 1);
        caster->SetMover(target);

        target->CombatStop(true);
        //target->DeleteThreatList();
        //target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
            ((Creature*)target)->AIM_Initialize();
        else if (target->GetTypeId() == TYPEID_PLAYER)
            dynamic_cast<Player*>(target)->SetClientControl(target, 0);

        CharmInfo *charmInfo = target->InitCharmInfo(target);
        charmInfo->InitPossessCreateSpells();
        charmInfo->SetReactState( REACT_PASSIVE );
        charmInfo->SetCommandState(COMMAND_STAY);

        MotionMaster* target_motionmaster = target->GetMotionMaster();
        if (target->GetTypeId() == TYPEID_UNIT && target_motionmaster)
        {
            target_motionmaster->Clear(true, true);
            target_motionmaster->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), false);
        }


        if(caster->GetTypeId() == TYPEID_PLAYER)
            caster->PossessSpellInitialize();
    }
    else
    {
        target->SetCharmerGuid(ObjectGuid());

        if (target->GetTypeId() == TYPEID_UNIT)
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();

            // restore faction
            if(cinfo)
                target->setFaction(cinfo->faction_A);
        }
        else if (target->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)target)->setFactionForRace(target->getRace());
        }


        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        caster->SetCharm(NULL);

        if(caster->GetTypeId() == TYPEID_PLAYER)
            caster->RemovePetActionBar();

        player_camera.SetView(caster);
        caster->SetClientControl(caster, 1);
        caster->SetMover(caster);

        target->CombatStop(true);
        //target->DeleteThreatList();
        //target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
            if (((Creature*)target)->AI())
                ((Creature*)target)->AI()->AttackedBy(caster);
        }
        else if (target->GetTypeId() == TYPEID_PLAYER)
            dynamic_cast<Player*>(target)->SetClientControl(target, 1);
    }
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* target = GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !((Creature*)target)->IsPet())
        return;

    Pet* pet = (Pet*)target;

    Player* p_caster = (Player*)caster;
    Camera& camera = p_caster->GetCamera();

    if (apply)
    {
        // This flag makes the aggro go to the controller instead of the controlled creature.
        // The flag doesn't seem necessary.
        //pet->addUnitState(UNIT_STAT_CONTROLLED);

        // target should became visible at SetView call(if not visible before):
        // otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
        camera.SetView(pet);

        p_caster->SetCharm(pet);
        p_caster->SetClientControl(pet, 1);
        ((Player*)caster)->SetMover(pet);

        pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        pet->StopMoving();
        pet->GetMotionMaster()->Clear(false);
        pet->GetMotionMaster()->MoveIdle();
    }
    else
    {
        p_caster->SetCharm(NULL);
        p_caster->SetClientControl(pet, 0);
        p_caster->SetMover(NULL);

        // there is a possibility that target became invisible for client\p_caster at ResetView call:
        // it must be called after movement control unapplying, not before! the reason is same as at aura applying
        camera.ResetView();

        // on delete only do caster related effects
        if(m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        //pet->clearUnitState(UNIT_STAT_CONTROLLED);

        pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        pet->AttackStop();

        // out of range pet dismissed
        if (!pet->IsWithinDistInMap(p_caster, pet->GetMap()->GetVisibilityDistance()))
        {
            p_caster->RemovePet(PET_SAVE_REAGENTS);
        }
        else
        {
            pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            pet->AddSplineFlag(SPLINEFLAG_WALKMODE);
        }
    }
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    // not charm yourself
    if (GetCasterGuid() == target->GetObjectGuid())
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if( apply )
    {
        // is it really need after spell check checks?
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM, GetHolder());
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS, GetHolder());

        target->SetCharmerGuid(GetCasterGuid());
        target->setFaction(caster->getFaction());
        target->CastStop(target == caster ? GetId() : 0);
        caster->SetCharm(target);

        target->CombatStop(true);
        //target->DeleteThreatList();
        //target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
            CharmInfo *charmInfo = target->InitCharmInfo(target);
            charmInfo->InitCharmCreateSpells();
            charmInfo->SetReactState( REACT_DEFENSIVE );

            if(caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();
                if(cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if(target->GetByteValue(UNIT_FIELD_BYTES_0, 1)==0)
                    {
                        if(cinfo->unit_class==0)
                            sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.",cinfo->Entry);
                        else
                            sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.",cinfo->Entry,cinfo->unit_class);

                        target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
                }
            }
        }
        else if(target->GetTypeId() == TYPEID_PLAYER)  // For the Gnomish Mind Control Cap
        {
            CharmInfo* playerCharmInfo = target->InitCharmInfo(target);
            playerCharmInfo->InitPossessCreateSpells();

            playerCharmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);

			// A player afflicted by the Chromatic Mutation at Chromaggus should be aggressive.
			if (GetId() == 23174)
			{
				playerCharmInfo->SetReactState(REACT_AGGRESSIVE);
			}
			else
			{
				playerCharmInfo->SetReactState(REACT_DEFENSIVE);
			}

            Player* targetPlayer = dynamic_cast<Player*>(target);

            targetPlayer->SetClientControl(target, 0);
            targetPlayer->SetMover(caster);
            caster->SetCharm(target);

            targetPlayer->GetMotionMaster()->Clear();
            targetPlayer->GetMotionMaster()->MoveFollow(caster, 0.5f, 3.14f / 6.0f);

            targetPlayer->SetCharmedCDTimer(3000);

            targetPlayer->CreateAI();

            target->setFaction(caster->getFaction());
        }

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->CharmSpellInitialize();
    }
    else
    {
        target->SetCharmerGuid(ObjectGuid());

        if(target->GetTypeId() == TYPEID_PLAYER)  // For the Gnomish Mind Control Cap
        {
            Player* targetPlayer = dynamic_cast<Player*>(target);

            targetPlayer->GetMotionMaster()->Clear();

            targetPlayer->DeleteAI();

            targetPlayer->SetClientControl(target, 1);
            targetPlayer->SetMover(NULL);
            targetPlayer->setFactionForRace(target->getRace());
        }
        else
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();

            // restore faction
            if(((Creature*)target)->IsPet())
            {
                if(Unit* owner = target->GetOwner())
                    target->setFaction(owner->getFaction());
                else if(cinfo)
                    target->setFaction(cinfo->faction_A);
            }
            else if(cinfo)                              // normal creature
                target->setFaction(cinfo->faction_A);

            // restore UNIT_FIELD_BYTES_0
            if(cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                // m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

                if(target->GetCharmInfo())
                    target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", target->GetGUIDLow(), target->GetTypeId());
            }
        }

        caster->SetCharm(NULL);

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->RemovePetActionBar();

        target->CombatStop(true);
        //target->DeleteThreatList();
        //target->getHostileRefManager().deleteReferences();

        if(target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)target)->AIM_Initialize();
            if (((Creature*)target)->AI())
                ((Creature*)target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if(!Real)
        return;

    GetTarget()->SetConfused(apply, GetCasterGuid(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    GetTarget()->SetFeared(apply, GetCasterGuid(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();

    if (!caster)
        return;

    if (apply)
    {

        // Interrup all spells being cast against the player.
        Unit::AttackerSet attackers(caster->getAttackers()); // A copy of the list is required since interrupting the spell being cast by the attacker may remove it from the list and cause a segfault if a reference is used.
        if (!attackers.empty())
        {
            std::for_each(attackers.begin(), attackers.end(), [&](Unit* current_attacker) {
                current_attacker->InterruptSpellsTargettingUnit(false, caster->GetObjectGuid());
            });
        }

    }

    GetTarget()->SetFeignDeath(apply, GetCasterGuid(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(!apply && target->HasAuraType(GetModifier()->m_auraname))
        return;

    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, apply);

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
    if (target->IsInFeralForm())
        return;

    if (apply)
        target->SetAttackTime(BASE_ATTACK,BASE_ATTACK_TIME);
    else
        ((Player *)target)->SetRegularAttackTime();

    target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(UNIT_STAT_STUNNED);
        target->SetTargetGuid(ObjectGuid());

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        target->CastStop(target->GetObjectGuid() == GetCasterGuid() ? GetId() : 0);

        // Creature specific
        if(target->GetTypeId() != TYPEID_PLAYER)
            target->StopMoving();
        else
        {
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_MOVE_STOP);
            target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
        }

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8);
        data << target->GetPackGUID();
        data << uint32(0);
        target->SendMessageToSet(&data, true);
    }
    else
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for(Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if( GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if(found_another)
                    break;
            }

            if(!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        target->clearUnitState(UNIT_STAT_STUNNED);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if(!target->hasUnitState(UNIT_STAT_ROOT))         // prevent allow move if have also root effect
        {
            if(target->getVictim() && target->isAlive())
                target->SetTargetGuid(target->getVictim()->GetObjectGuid());

            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
            data << target->GetPackGUID();
            data << uint32(0);
            target->SendMessageToSet(&data, true);
        }

        // Wyvern Sting
        if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellFamilyFlags & UI64LIT(0x00010000))
        {
            Unit* caster = GetCaster();
            if( !caster || caster->GetTypeId()!=TYPEID_PLAYER )
                return;

            uint32 spell_id = 0;

            switch(GetId())
            {
            case 19386:
                spell_id = 24131;
                break;
            case 24132:
                spell_id = 24134;
                break;
            case 24133:
                spell_id = 24135;
                break;
            default:
                sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?",GetId());
                return;
            }

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

            if(!spellInfo)
                return;

            caster->CastSpell(target,spellInfo,true,NULL,this);
            return;
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (GetId() == 9991)
        return;

    if (apply)
    {
        // drop flag at stealth in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);

            if (target->GetTypeId()==TYPEID_PLAYER)
                target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (target->GetVisibility()!=VISIBILITY_OFF)
            {
                target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // for RACE_NIGHTELF stealth
            if (target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
                target->CastSpell(target, 21009, true, NULL, this);
        }
    }
    else
    {
        // for RACE_NIGHTELF stealth
        if (Real && target->GetTypeId()==TYPEID_PLAYER && GetId() == 20580)
            target->RemoveAurasDueToSpell(21009);

        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !target->HasStealthAura())
        {
            // if no GM invisibility
            if (target->GetVisibility()!=VISIBILITY_OFF)
            {
                target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);

                if (target->GetTypeId()==TYPEID_PLAYER)
                    target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

                // restore invisibility if any
                if (target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
    {
        target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if (Real && target->GetTypeId()==TYPEID_PLAYER)
        {
            // apply glow vision
            target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        }

        // apply only if not in GM invisibility and not stealth
        if (target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_invisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if (Real && target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            // apply only if not in GM invisibility & not stealthed while invisible
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if(apply)
    {
        target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_detectInvisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);
    }
    if(Real && target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)target)->GetCamera().UpdateVisibilityForOwner();
}

void Aura::HandleDetectAmore(bool apply, bool /*real*/)
{
    GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES2, 2, PLAYER_FIELD_BYTE2_DETECT_AMORE, apply);
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // For the teleportation of rogues at Nefarian.
        if (GetId() == 23414)
        {
            float angle = frand(0, 2 * 3.141592654f);

            for (int i = 0; i <= 10; i++)
            {
                float x = GetCaster()->GetPositionX() +  12.f * cos(angle), y = GetCaster()->GetPositionY() + 12.f * sin(angle), z = GetCaster()->GetPositionZ();

                if (target->IsWithinLOS(x, y, z))
                    target->NearTeleportTo(x, y, z, 0);
                else
                    angle = frand(0, 2 * 3.141592654f);
            }

        }


        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(UNIT_STAT_ROOT);
        target->SetTargetGuid(ObjectGuid());

        //Save last orientation
        if( target->getVictim() )
            target->SetOrientation(target->GetAngle(target->getVictim()));

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
            data << target->GetPackGUID();
            data << (uint32)2;
            target->SendMessageToSet(&data, true);

            //Clear unit movement flags
            ((Player*)target)->m_movementInfo.SetMovementFlags(MOVEFLAG_MOVE_STOP);
        }
        else
            target->StopMoving();
    }
    else
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for(AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for(Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if( GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if(found_another)
                    break;
            }

            if(!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        target->clearUnitState(UNIT_STAT_ROOT);

        if(!target->hasUnitState(UNIT_STAT_STUNNED))      // prevent allow move if have also stun effect
        {
            if(target->getVictim() && target->isAlive())
                target->SetTargetGuid(target->getVictim()->GetObjectGuid());

            if(target->GetTypeId() == TYPEID_PLAYER)
            {
                WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 10);
                data << target->GetPackGUID();
                data << (uint32)2;
                target->SendMessageToSet(&data, true);
            }
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    Unit *target = GetTarget();

    if(apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if(spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive())
        return;

    int level_diff = 0;
    int multiplier = 0;
    switch (GetId())
    {
        // Arcane Shroud
    case 26400:
        level_diff = target->getLevel() - 60;
        multiplier = 2;
        break;
        // The Eye of Diminution
    case 28862:
        level_diff = target->getLevel() - 60;
        multiplier = 1;
        break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (target->GetTypeId() == TYPEID_PLAYER)
        for(int8 x=0; x < MAX_SPELL_SCHOOL; x++)
            if (m_modifier.m_miscvalue & int32(1<<x))
                ApplyPercentModFloatVar(target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    float threatMod = apply ? float(m_modifier.m_amount) : float(-m_modifier.m_amount);

    target->getHostileRefManager().threatAssist(caster, threatMod, GetSpellProto(), true);
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || !target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    if (apply)
        target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        target->TauntFadeOut(caster);
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit* target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
    uint32 misc  = m_modifier.m_miscvalue;
    Unit *target = GetTarget();

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        uint32 mechanic = 1 << (misc-1);

        target->RemoveAurasAtMechanicImmunity(mechanic,GetId());
    }

    target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,misc,apply);
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        GetTarget()->RemoveAurasAtMechanicImmunity(mechanic,GetId());

    // check implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // when removing flag aura, handle flag drop
    if( !apply && target->GetTypeId() == TYPEID_PLAYER
            && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION) )
    {
        if( BattleGround *bg = ((Player*)target)->GetBattleGround() )
            bg->EventPlayerDroppedFlag(((Player*)target));
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if(apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = GetTarget()->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for(Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                GetTarget()->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    Unit* target = GetTarget();

    // If the Giantstalker's 5/8-bonus, Nature's Ally, is already applied do not reapply or remove it.
    if (GetId() == 21925)
    {
        Pet* pet = dynamic_cast<Pet*>(GetTarget());
        if (pet)
        {
            if (apply)
            {
                if (pet->IsGiantstalkerBuffApplied())
                    return;
            }
            else
            {
                if (m_removeMode != AuraRemoveMode::AURA_REMOVE_BY_DEATH)
                {
                    Unit* owner = pet->GetCharmerOrOwner();
                    if (owner && owner->HasAura(21926))
                        return;
                }
                else
                    pet->SetGiantstalkerBuffApplied(false);
            }
        }
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if( GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
            && GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD )
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if( Real && apply
            && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
            && IsPositiveSpell(GetId()) )                       //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::SpellAuraHolderMap& Auras = target->GetSpellAuraHolderMap();
        for(Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if((GetSpellSchoolMask(spell) & school_mask)//Check for school mask
                    && !( spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)   //Spells unaffected by invulnerability
                    && !iter->second->IsPositive()          //Don't remove positive spells
                    && spell->Id != GetId() )               //Don't remove self
            {
                target->RemoveAurasDueToSpell(spell->Id);
                if(Auras.empty())
                    break;
                else
                    next = Auras.begin();
            }
        }
    }
    if( Real && GetSpellProto()->Mechanic == MECHANIC_BANISH )
    {
        if( apply )
            target->addUnitState(UNIT_STAT_ISOLATED);
        else
            target->clearUnitState(UNIT_STAT_ISOLATED);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if(!Real)
        return;

    switch (GetId())
    {
        // some spell have charges by functionality not have its in spell data
    case 28200:                                         // Ascendance (Talisman of Ascendance trinket)
        if(apply)
            GetHolder()->SetAuraCharges(6);
        break;
    default:
        break;
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if(apply)
        GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
        GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    if (!apply)
    {
        switch(GetId())
        {
        case 21926: // Make sure the pet's aura is removed when the Giantstalker's 5/8-bonus, Nature's Ally, is broken.
        {
            Pet* pet = target->GetPet();
            if (pet && pet->HasAura(21925))
            {
                SpellAuraHolder* aura_holder = pet->GetSpellAuraHolder(21925);
                if (aura_holder)
                    pet->RemoveSpellAuraHolder(aura_holder, AuraRemoveMode::AURA_REMOVE_BY_DEATH);
            }
            break;
        }
        case 29213:                                     // Curse of the Plaguebringer
            if (m_removeMode != AURA_REMOVE_BY_DISPEL)
                // Cast Wrath of the Plaguebringer if not dispelled
                target->CastSpell(target, 29214, true, 0, this);
            return;
        default:
            break;
        }
    }
    else
    {
        switch(GetId())
        {
        case 21926: // For the Giantstalker 5/8-bonus.
            m_modifier.periodictime = 2000;
            break;
        default:
            break;
        }
    }
}

void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicEnergize(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    if (GetId() == 5229 && apply)		//Druid enrage in bear form
    {
        Aura* A = CreateAura(GetSpellProto(),EFFECT_INDEX_1,0,m_spellAuraHolder,GetTarget(),GetCaster());
        m_spellAuraHolder->AddAura(A,EFFECT_INDEX_1);
        if (GetTarget() && GetTarget()->HasAura(9635))		//-16% armor in dire bear form
            A->m_modifier.m_amount = -16;
        else
            A->m_modifier.m_amount = -27;
        A->m_modifier.m_miscvalue = SPELL_SCHOOL_MASK_NORMAL;
        A->m_modifier.periodictime = 0;
        A->m_modifier.m_auraname = SPELL_AURA_MOD_BASE_RESISTANCE_PCT;
    }
}

void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    Unit *caster = GetCaster();
    if (!caster)
        return;

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());

        Unit::AuraList const& caster_auras = caster->GetAurasByType(AuraType::SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);					// For the druids' Idol of Rejuvenation.
        for (Aura* current_aura : caster_auras)
            if (current_aura->GetId() == 27853)
                m_modifier.m_amount += (int)(50 * (float) m_modifier.periodictime / (float) GetAuraMaxDuration());
    }
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    m_isPeriodic = apply;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    Unit *caster = GetCaster();
    if (!caster)
        return;

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        switch (spellProto->SpellFamilyName)
        {
        case SPELLFAMILY_DRUID:
        {
            // Rip
            if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000800000))
            {
                // $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    uint8 cp = ((Player*)caster)->GetComboPoints();

                    if (cp > 4) cp = 4;
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Rupture
            if (spellProto->SpellFamilyFlags & UI64LIT(0x000000000000100000))
            {
                // Dmg/tick = $AP*min(0.01*$cp, 0.03) [Like Rip: only the first three CP increase the contribution from AP]
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    uint8 cp = ((Player*)caster)->GetComboPoints();
                    if (cp > 3) cp = 3;
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                }
            }
            break;
        }
        default:
            break;
        }

        if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
        {
            // SpellDamageBonusDone for magic spells
            if(spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                m_modifier.m_amount = caster->SpellDamageBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
            // MeleeDamagebonusDone for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
                m_modifier.m_amount = caster->MeleeDamageBonusDone(target, m_modifier.m_amount, attackType, GetSpellProto(), DOT, GetStackAmount());
            }

            Unit::AuraList const& caster_auras = caster->GetAurasByType(AuraType::SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);		// For the druids' Idol of the Moon.
            for (Aura* current_aura : caster_auras)
                if (current_aura->GetId() == 28854)
                    m_modifier.m_amount *= 1.17;
        }
    }
    else
    {
        // Handle Doomguard Summoning for the Curse of Doom.
        if (GetId() == 603)
        {
            if (target)
            {
                if (urand(0, 99) < 5 && (int32) target->GetHealth() <= m_modifier.m_amount)
                    target->CastSpell(target, 18662, true);
            }
        }
    }

    // Temp fix for Hakkar's Blood Siphon - disable control for player
    if (GetId() == 24323 && caster->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)caster)->SetClientControl(caster, !apply);
        if (apply)
            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        else
            caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
    }
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }

    // Temp fix for Hakkar's Blood Siphon - disable control for player
    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    if (GetId() == 24322)
    {
        ((Player*)caster)->SetClientControl(caster, !apply);
        if (apply)
            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        else
            caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
    }
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

bool Aura::IsMorePowerfulResistanceApplied(int8 currentSchoolmask)
{
    Unit *target = GetTarget();

    Unit::AuraList const& resistanceAuras = target->GetAurasByType(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE);


    for (auto itr = resistanceAuras.begin(); itr != resistanceAuras.end(); ++itr)
    {
        Aura *current_aura = *itr;
        if (currentSchoolmask & current_aura->m_modifier.m_miscvalue && m_modifier.m_amount < current_aura->m_modifier.m_amount && current_aura != this)
            return true;
    }


    return false;

}

Aura* Aura::GetMostPowerfulResistance(int8 currentSchoolmask)
{
    Unit *target = GetTarget();
    Aura* mostPowerful = NULL;

    Unit::AuraList const& resistanceAuras = target->GetAurasByType(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE);

    if (resistanceAuras.empty())
        return NULL;


    for (auto itr = resistanceAuras.begin(); itr != resistanceAuras.end(); ++itr)
    {
        Aura *current_aura = *itr;

        if (mostPowerful == NULL && currentSchoolmask & current_aura->m_modifier.m_miscvalue && current_aura != this)
            mostPowerful = current_aura;

        if (!mostPowerful)
            continue;

        if (currentSchoolmask & current_aura->m_modifier.m_miscvalue && mostPowerful->m_modifier.m_amount < current_aura->m_modifier.m_amount && current_aura != this)
            mostPowerful = current_aura;
    }

    return mostPowerful;
}

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            if (!IsMorePowerfulResistanceApplied(int32(1<<x)) && apply)
            {
                Aura* mostPowerfulResistance = GetMostPowerfulResistance(int32(1<<x)); // If the new resistance mod is more powerful remove the old most powerful mod and replace it with the new one.
                if (mostPowerfulResistance)
                {
                    GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(mostPowerfulResistance->m_modifier.m_amount), false);
                    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                        ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(mostPowerfulResistance->m_modifier.m_amount), false);
                }

                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply); // Apply the new mod.
                if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                    ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
            }
            else if (!apply && !IsMorePowerfulResistanceApplied(int32(1<<x)))
            {

                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply); // If this mod was the most powerful applied remove it.
                if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                    ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);

                Aura* mostPowerfulResistance = GetMostPowerfulResistance(int32(1<<x));                                                              // And apply the new most powerful mod.
                if (mostPowerfulResistance)
                {
                    GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(mostPowerfulResistance->m_modifier.m_amount), true);
                    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                        ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(mostPowerfulResistance->m_modifier.m_amount), true);
                }
            }
        }

    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For the Zandalarian Hero badge.
    if (GetSpellProto()->Id == 24575 && GetAuraDuration() / GetAuraMaxDuration() >= 1.f)
    {
        if (m_modifier.m_amount == 200)
            m_modifier.m_amount = 2000;
    }
    else if (GetId() == 21992) // Extra threat for Thunderfury.
    {
        Creature* cTarget = dynamic_cast<Creature*>(target);
        if (cTarget)
        {
            ThreatManager& mgr = cTarget->getThreatManager();
            Unit* caster = GetCaster();
            
            if (caster)
                mgr.addThreatDirectly(caster, 216.f);
        }
    }

    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }

    // Faerie Fire (druid versions)
    if( spellProto->SpellIconID == 109 &&
            spellProto->SpellFamilyName == SPELLFAMILY_DRUID &&
            spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400))
    {
        target->ApplySpellDispelImmunity(spellProto, DISPEL_STEALTH, apply);
        target->ApplySpellDispelImmunity(spellProto, DISPEL_INVISIBILITY, apply);
    }
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if(((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
        {
            if(m_modifier.m_miscvalue & int32(1<<x))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleAurasVisible(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_AURAS_VISIBLE, apply);
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    for(int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if(m_modifier.m_miscvalue & int32(1<<i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(target->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), m_positive, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    Pet* pet = dynamic_cast<Pet*>(GetTarget());

    // If the Giantstalker's 5/8-bonus, Nature's Ally, is already applied do not reapply or remove it.
    if (GetId() == 21925)
    {
        if (pet && !pet->IsGiantstalkerBuffApplicable(this, apply))
            return;
    }

    // Only players and pets have base stats.
    if(GetTarget()->GetTypeId() == TYPEID_PLAYER || pet)
    {
        for(int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if(m_modifier.m_miscvalue & (1<<i))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ",GetId(),GetEffIndex(),m_modifier.m_miscvalue);
        return;
    }

    // Custom calculation for the Well Fed buff for the Bobbing Apples.
    if (GetId() == 24870)
        m_modifier.m_amount = (int)(GetTarget()->getLevel() * 0.25f);

    // If the Giantstalker's 5/8-bonus, Nature's Ally, is already applied do not reapply or remove it.
    if (GetId() == 21925)
    {
        Pet* pet = dynamic_cast<Pet*>(GetTarget());
        if (pet && !pet->IsGiantstalkerBuffApplicable(this, apply))
            return;
    }

    // Hunter talent Improved Scorpid Sting
    if ((GetId() == 14277 || GetId() == 3043 || GetId() == 14275 || GetId() == 14276)
        && m_modifier.m_miscvalue == STAT_STRENGTH)
    {
        Unit* caster = GetCaster();
        if (caster)
        {
            float effect = 0;
            if (caster->HasAura(19491))
                effect = 0.1f * m_modifier.m_amount;
            else if (caster->HasAura(19493))
                effect = 0.2f * m_modifier.m_amount;
            else if (caster->HasAura(19494))
                effect = 0.3f * m_modifier.m_amount;
            
            if (apply)
                m_modifier.m_custom = effect;
            
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + STAT_STAMINA), TOTAL_VALUE, float(m_modifier.m_custom), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                ((Player*)GetTarget())->ApplyStatBuffMod(Stats(STAT_STAMINA), float(m_modifier.m_custom), apply);
        }
    }
    

    for(int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            //m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
                ((Player*)GetTarget())->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Fix for the Zandalaran Hero Charm not giving the correct bonus on the first value.
    if (GetSpellProto()->Id == 24659 && GetAuraDuration() / GetAuraMaxDuration() > 0.9f)
    {
        if( m_modifier.m_amount == 34)
            m_modifier.m_amount = 408;
    }

    // implemented in Unit::SpellHealingBonusDone
    // this information is for client side only
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    Unit *target = GetTarget();

    //save current and max HP before applying aura
    uint32 curHPValue = target->GetHealth();
    uint32 maxHPValue = target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)target)->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply );
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (GetSpellProto()->Attributes & 0x10))
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
        target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    GetTarget()->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if(m_modifier.periodictime == 0)
        m_modifier.periodictime = 1000;

    m_periodicTimer = m_modifier.periodictime;
    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if(m_modifier.periodictime == 0)
        m_modifier.periodictime = 5000;

    m_periodicTimer = 5000;
    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if (!Real)
        return;

    Powers pt = GetTarget()->getPowerType();
    if(m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (pt == POWER_RAGE)
            m_modifier.periodictime = 3000;
        else
            m_modifier.periodictime = 2000;
    }

    m_periodicTimer = 5000;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // Special case with temporary increase max/current health
    switch(GetId())
    {
    case 1178:                                          // Bear Form (Passive)
    case 9635:                                          // Dire Bear Form (Passive)
    {
        if(Real)
        {
            float pct = target->GetHealthPercent();
            target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->SetHealthPercent(pct);
        }
        return;
    }
    case 12976:                                         // Warrior Last Stand triggered spell
    {
        if(Real)
        {
            if(apply)
            {
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                target->ModifyHealth(m_modifier.m_amount);
            }
            else
            {
                if (int32(target->GetHealth()) > m_modifier.m_amount)
                    target->ModifyHealth(-m_modifier.m_amount);
                else
                    target->SetHealth(1);
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            }
        }
        return;
    }
	case 23782: // Lifegiving Gem
	{
		if (apply)
			m_modifier.m_amount = float(target->GetMaxHealth() * 0.15f);
	}
    }

    // generic case
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    Powers powerType = target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = GetTarget()->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    GetTarget()->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if(target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if(Real)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // Biznicks 247x128 Accurascope should not give melee hit.
    if (GetId() != 22780)
        target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    
    target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount: (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateAllSpellCritChances();
    }
    else
    {
        GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount:(-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for(int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1<<school))
            ((Player*)GetTarget())->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyCastTimePercentMod(float(m_modifier.m_amount),apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(BASE_ATTACK,float(m_modifier.m_amount),apply);
}

void Aura::HandleModMeleeSpeedPct(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    
    // Thunderfury extra threat.
    if (GetId() == 27648)
    {
        Creature* cTarget = dynamic_cast<Creature*>(target);
        if (cTarget)
        {
            ThreatManager& mgr = cTarget->getThreatManager();
            Unit* caster = GetCaster();
            if (caster)
                mgr.addThreatDirectly(caster, 134.f);
        }
    }
    
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if(Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    // Fix for the Zandalaran Hero Charm not giving the correct bonus on the first value.
    if (GetSpellProto()->Id == 24659 && GetAuraDuration() / GetAuraMaxDuration() > 0.9f)
    {
        if( m_modifier.m_amount == 17)
            m_modifier.m_amount = 204;
    }

    // Fix for the Zandalarian Hero Medallion.
    if (GetSpellProto()->Id == 24662 && GetAuraDuration() / GetAuraMaxDuration() > 0.9f)
    {
        if (m_modifier.m_amount == 2)
            m_modifier.m_amount = 40;
    }


    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if(target->GetTypeId() == TYPEID_PLAYER)
        {
            if(m_positive)
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
        }
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    if(target->GetTypeId() == TYPEID_PLAYER)
    {
        if(m_positive)
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = target->GetPet();
        if(pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if(Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if(target->GetTypeId() == TYPEID_PLAYER)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount/100.0f, apply);
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
    // Send info to client
    if(target->GetTypeId() == TYPEID_PLAYER)
        for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount/100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

    GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    float amount = m_modifier.m_amount/100.0f;
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 HotWSpellId = 0;

    ShapeshiftForm form = ShapeshiftForm(GetModifier()->m_miscvalue);

    Unit *target = GetTarget();

    switch(form)
    {
    case FORM_CAT:
        spellId1 = 3025;
        HotWSpellId = 24900;
        break;
    case FORM_TREE:
        spellId1 = 5420;
        break;
    case FORM_TRAVEL:
        spellId1 = 5419;
        break;
    case FORM_AQUA:
        spellId1 = 5421;
        break;
    case FORM_BEAR:
        spellId1 = 1178;
        spellId2 = 21178;
        HotWSpellId = 24899;
        break;
    case FORM_DIREBEAR:
        spellId1 = 9635;
        spellId2 = 21178;
        HotWSpellId = 24899;
        break;
    case FORM_BATTLESTANCE:
        spellId1 = 21156;
        break;
    case FORM_DEFENSIVESTANCE:
        spellId1 = 7376;
        break;
    case FORM_BERSERKERSTANCE:
        spellId1 = 7381;
        break;
    case FORM_MOONKIN:
        spellId1 = 24905;
        break;
    case FORM_SPIRITOFREDEMPTION:
        spellId1 = 27792;
        spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
        break;
    case FORM_GHOSTWOLF:
    case FORM_AMBIENT:
    case FORM_GHOUL:
    case FORM_SHADOW:
    case FORM_STEALTH:
    case FORM_CREATURECAT:
    case FORM_CREATUREBEAR:
        break;
    default:
        break;
    }

    if(apply)
    {
        if (spellId1)
            target->CastSpell(target, spellId1, true, NULL, this );
        if (spellId2)
            target->CastSpell(target, spellId2, true, NULL, this);

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first==spellId1 || itr->first==spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !IsNeedCastSpellAtFormApply(spellInfo, form))
                    continue;
                target->CastSpell(target, itr->first, true, NULL, this);
            }

            // Leader of the Pack
            if (((Player*)target)->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->Stances & (1<<(form-1)))
                    target->CastSpell(target, 24932, true, NULL, this);
            }

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for(Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if(spellId1)
            target->RemoveAurasDueToSpell(spellId1);
        if(spellId2)
            target->RemoveAurasDueToSpell(spellId2);

        Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
                ++itr;
        }
    }
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(GetTarget()->GetEntry());
    if(ci && ci->type == CREATURE_TYPE_BEAST)
        GetTarget()->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if(apply)
        GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if (apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
    {
        GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
    else
    {
        GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if(m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if(GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)GetTarget();

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!apply && m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetComboTargetGuid())
        if (Unit* unit = ObjectAccessor::GetUnit(*GetTarget(),target->GetComboTargetGuid()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable( bool Apply, bool Real )
{
    if(Real && Apply)
    {
        GetTarget()->CombatStop();
        GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE,Apply);
}

void Aura::HandleSpiritOfRedemption( bool apply, bool Real )
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    Unit *target = GetTarget();

    // prepare spirit state
    if(apply)
    {
        if(target->GetTypeId()==TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if(!target->IsStandState())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
        target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    if (apply)
    {
        // prevent double apply bonuses
        if (target->GetTypeId()!=TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch(spellProto->SpellFamilyName)
            {
            case SPELLFAMILY_PRIEST:
                // Power Word: Shield
                if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                {
                    //+30% from +healing bonus
                    DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.3f;
                    break;
                }
                break;
            case SPELLFAMILY_MAGE:
                // Frost ward, Fire ward
                if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000100080108)))
                {
                    //+10% from +spd bonus
                    DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                    break;
                }
                break;
            case SPELLFAMILY_WARLOCK:
                if (spellProto->SpellFamilyFlags == UI64LIT(0x00))
                {
                    //shadow ward
                    //+10% from +spd bonus
                    DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                    break;
                }
                break;
            default:
                break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::PeriodicTick()
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    switch(m_modifier.m_auraname)
    {
    case SPELL_AURA_PERIODIC_DAMAGE:
    case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
    {
        // don't damage target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        Unit *pCaster = GetCaster();
        if(!pCaster)
            return;

        PartialResistInfo partialResist;
        SpellMissInfo missInfo = pCaster->SpellHitResult(target, spellProto, partialResist,false);

        if( spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA && missInfo != SPELL_MISS_NONE)
            return;

        // Check for immune (not use charges)
        if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
            return;

        uint32 absorb = 0;
        uint32 resist = 0;
        CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

        // ignore non positive values (can be result apply spellmods to aura damage
        uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        uint32 pdamage;

        if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            pdamage = amount;
        else
            pdamage = uint32(target->GetMaxHealth()*amount/100);

        // SpellDamageBonus for magic spells
        if(spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
            pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());
        // MeleeDamagebonus for weapon based spells
        else
        {
            WeaponAttackType attackType = GetWeaponAttackType(spellProto);
            pdamage = target->MeleeDamageBonusTaken(pCaster, pdamage, attackType, spellProto, DOT, GetStackAmount());
        }

        // Calculate armor mitigation if it is a physical spell
        // But not for bleed mechanic spells
        if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL &&
                GetEffectMechanic(spellProto, m_effIndex) != MECHANIC_BLEED)
        {
            uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
            cleanDamage.damage += pdamage - pdamageReductedArmor;
            pdamage = pdamageReductedArmor;
        }

        // Curse of Agony damage-per-tick calculation
        if (spellProto->SpellFamilyName==SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400)) && spellProto->SpellIconID==544)
        {
            // 1..4 ticks, 1/2 from normal tick damage
            if (GetAuraTicks() <= 4)
                pdamage = pdamage/2;
            // 9..12 ticks, 3/2 from normal tick damage
            else if(GetAuraTicks() >= 9)
                pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
            // 5..8 ticks have normal tick damage
        }

        // Engineering Gnomish Death Ray damage calculations.
        if (spellProto->Id == 13493)
        {
            CumulativeEffectAura* deathRayAura = dynamic_cast<CumulativeEffectAura*>(pCaster->GetAura(13278, EFFECT_INDEX_0));

            if (!deathRayAura) // Check if the death ray has been interrupted.
            {
                pCaster->RemoveAura(13493, EFFECT_INDEX_0);
                return;
            }

            float damageModifier = deathRayAura->GetDamageModifier();

            pdamage = pdamage * ( (float)(pCaster->GetMaxHealth()) * damageModifier / 150.0f );  // Modify the damage to be around 7,5 - 9,0 % of total HP.
            deathRayAura->AddCumulativeEffect(pdamage);  // Add to the total amount of damage done for the final damage calculations.;

        }

        PartialResistInfo partial = pCaster->MagicSpellPartialResistResult(target,spellProto);
        target->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !(spellProto->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED),partial);

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s attacked %s for %u dmg inflicted by %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

        pCaster->DealDamageMods(target, pdamage, &absorb);

        // Set trigger flag
        uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
        uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
        pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);

        // Hunter traps spells (for Entrapment trigger)
        // Runs Immolation Trap, Frost Trap, Explosive Trap procs
        if (spellProto->SpellFamilyName == SPELLFAMILY_HUNTER && spellProto->SpellFamilyFlags & UI64LIT(0x000020000000001C))
            procAttacker |= PROC_FLAG_ON_TRAP_ACTIVATION;

        SpellPeriodicAuraLogInfo pInfo(this, pdamage, absorb, resist, 0.0f);
        target->SendPeriodicAuraLog(&pInfo);

        if (pdamage)
            procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;

        pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, 0, BASE_ATTACK, spellProto);

        pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, true);
        break;
    }
    case SPELL_AURA_PERIODIC_LEECH:
    case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
    {
        // don't damage or heal target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        Unit *pCaster = GetCaster();
        if(!pCaster)
            return;

        if(!pCaster->isAlive())
            return;

        //To do: find out what spells are supposed to heal target
        Unit* pDamageTarget = target;
        Unit* pHealTarget = pCaster;
        if (GetSpellProto()->Id == 24322)		//Blood Siphon
        {
            pDamageTarget = pCaster;
            pHealTarget = target;
        }

        PartialResistInfo partialResist;
        SpellMissInfo missInfo = pCaster->SpellHitResult(pDamageTarget, spellProto, partialResist, false);
        if( spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA && missInfo != SPELL_MISS_NONE)
            return;

        // Check for immune
        if(pDamageTarget->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
            return;

        uint32 absorb=0;
        uint32 resist=0;
        CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

        uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        //Calculate armor mitigation if it is a physical spell
        if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL)
        {
            uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(pDamageTarget, pdamage);
            cleanDamage.damage += pdamage - pdamageReductedArmor;
            pdamage = pdamageReductedArmor;
        }

        pdamage = pDamageTarget->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

        pDamageTarget->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, !(spellProto->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

        if(pDamageTarget->GetHealth() < pdamage)
            pdamage = uint32(pDamageTarget->GetHealth());

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s health leech of %s for %u dmg inflicted by %u abs is %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId(),absorb);

        pCaster->DealDamageMods(pDamageTarget, pdamage, &absorb);

        pCaster->SendSpellNonMeleeDamageLog(pDamageTarget, GetId(), pdamage, GetSpellSchoolMask(spellProto), absorb, resist, false, 0);

        float multiplier = spellProto->EffectMultipleValue[GetEffIndex()] > 0 ? spellProto->EffectMultipleValue[GetEffIndex()] : 1;

        // Set trigger flag
        uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;  // | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
        uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;

        pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);
        if (pdamage)
            procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;

        pCaster->ProcDamageAndSpell(pDamageTarget, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, 0, BASE_ATTACK, spellProto);
        int32 new_damage = pCaster->DealDamage(pDamageTarget, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, false);

        if (!target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
            for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                    if (spell->m_spellInfo->Id == GetId())
                        spell->cancel();

        if(Player *modOwner = pCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

        uint32 heal = pHealTarget->SpellHealingBonusTaken(pCaster, spellProto, int32(new_damage * multiplier), DOT, GetStackAmount());
        int32 gain = pCaster->DealHeal(pHealTarget, heal, spellProto);

        if (pHealTarget == pCaster)
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
        break;
    }
    case SPELL_AURA_PERIODIC_HEAL:
    case SPELL_AURA_OBS_MOD_HEALTH:
    {
        // don't heal target if not alive, mostly death persistent effects from items
        if (!target->isAlive())
            return;

        Unit *pCaster = GetCaster();
        if(!pCaster)
            return;

        // ignore non positive values (can be result apply spellmods to aura damage
        uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        uint32 pdamage;

        if(m_modifier.m_auraname==SPELL_AURA_OBS_MOD_HEALTH)
            pdamage = uint32(target->GetMaxHealth() * amount / 100);
        else
            pdamage = amount;

        pdamage = target->SpellHealingBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s heal of %s for %u health inflicted by %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());
        int32 gain = target->ModifyHealth(pdamage);
        SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
        target->SendPeriodicAuraLog(&pInfo);

        // Set trigger flag
        uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC;
        uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;
        uint32 procEx = PROC_EX_NORMAL_HIT | PROC_EX_PERIODIC_POSITIVE;
        pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, gain,0, BASE_ATTACK, spellProto);

        target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);

        // The Well Fed buff from the Bobbing Apples.
        if ( GetId() == 24869 && !target->HasAura(24870) && GetAuraDuration() <= 14000)
            target->CastSpell(target, 24870, true);
        
        // Hunter talent Improved Mend Pet
        if (spellProto->SpellIconID == 267 && spellProto->IsFitToFamily(SPELLFAMILY_HUNTER, uint64(0x800000)))
        {
            uint32 chance = 0;
            if (pCaster->HasAura(19572))
                chance = 15;
            else if (pCaster->HasAura(19573))
                chance = 50;
            
            if (urand(0, 99) < chance)
                pCaster->CastSpell(pCaster, 24406, true);
        }

        break;
    }
    case SPELL_AURA_PERIODIC_MANA_LEECH:
    {
        // don't damage target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
            return;

        Powers power = Powers(m_modifier.m_miscvalue);

        // power type might have changed between aura applying and tick (druid's shapeshift)
        if(target->getPowerType() != power)
            return;

        Unit *pCaster = GetCaster();
        if(!pCaster)
            return;

        if(!pCaster->isAlive())
            return;

        PartialResistInfo partialResist;
        SpellMissInfo missInfo = pCaster->SpellHitResult(target, spellProto, partialResist, false);
        if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA && missInfo != SPELL_MISS_NONE)
            return;

        // Check for immune (not use charges)
        if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
            return;

        // ignore non positive values (can be result apply spellmods to aura damage
        uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s power leech of %s for %u dmg inflicted by %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

        int32 drain_amount = target->GetPower(power) > pdamage ? pdamage : target->GetPower(power);

        target->ModifyPower(power, -drain_amount);

        // For the Warlock's Improved Mana Drain talent.
        float damage_coeff = 0;
        if (pCaster->HasAura(17864))
            damage_coeff = 0.15f;
        else if (pCaster->HasAura(18393))
            damage_coeff = 0.30f;

        if (damage_coeff)
            pCaster->DealDamage(target, (uint32)(drain_amount * damage_coeff), NULL, DOT, SPELL_SCHOOL_MASK_SHADOW, spellProto, true);

        float gain_multiplier = 0;

        if(pCaster->GetMaxPower(power) > 0)
        {
            gain_multiplier = spellProto->EffectMultipleValue[GetEffIndex()];

            if(Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
        }

        SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, gain_multiplier);
        target->SendPeriodicAuraLog(&pInfo);

        int32 gain_amount = int32(drain_amount * gain_multiplier);

        if(gain_amount)
        {
            int32 gain = pCaster->ModifyPower(power, gain_amount);
            target->AddThreat(pCaster, float(gain) * 0.5f, false, GetSpellSchoolMask(spellProto), spellProto);
        }

        //Mark of Kazzak: Explode
        if (GetId() == 21056 && target->GetPower(POWER_MANA) < 250)
        {
            target->CastSpell(target,21058,true,0,this,pCaster->GetObjectGuid(),0);
            target->RemoveAurasDueToSpell(21056);
        }

        break;
    }
    case SPELL_AURA_PERIODIC_ENERGIZE:
    {
        // don't energize target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        // ignore non positive values (can be result apply spellmods to aura damage
        uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u dmg inflicted by %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

        if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
            break;

        Powers power = Powers(m_modifier.m_miscvalue);

        if(target->GetMaxPower(power) == 0)
            break;

        SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
        target->SendPeriodicAuraLog(&pInfo);

        if(GetId() == 29131 || GetId() == 5229) // Bloodrage & Enrage
        {
            if(target->HasAura(29131) || target->HasAura(5229)) // If player has aura.
            {
                target->SetInCombatState(false, target); // Put player in combat.
            }
        }

        int32 gain = target->ModifyPower(power,pdamage);

        if(Unit* pCaster = GetCaster())
            target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
        break;
    }
    case SPELL_AURA_OBS_MOD_MANA:
    {
        // don't energize target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        // ignore non positive values (can be result apply spellmods to aura damage
        uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        uint32 pdamage = uint32(target->GetMaxPower(POWER_MANA) * amount / 100);

        DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u mana inflicted by %u",
                          GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

        if(target->GetMaxPower(POWER_MANA) == 0)
            break;

        SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
        target->SendPeriodicAuraLog(&pInfo);

        int32 gain = target->ModifyPower(POWER_MANA, pdamage);

        if(Unit* pCaster = GetCaster())
            target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
        break;
    }
    case SPELL_AURA_POWER_BURN_MANA:
    {
        // don't mana burn target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        Unit *pCaster = GetCaster();
        if(!pCaster)
            return;

        // Check for immune (not use charges)
        if(target->IsImmunedToDamage(GetSpellSchoolMask(spellProto)))
            return;

        int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

        Powers powerType = Powers(m_modifier.m_miscvalue);

        if(!target->isAlive() || target->getPowerType() != powerType)
            return;

        uint32 gain = uint32(-target->ModifyPower(powerType, -pdamage));

        gain = uint32(gain * spellProto->EffectMultipleValue[GetEffIndex()]);

        // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
        SpellNonMeleeDamage damageInfo(pCaster, target, spellProto->Id, SpellSchools(spellProto->School));
        pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

        damageInfo.target->CalculateAbsorbResistBlock(pCaster, &damageInfo, spellProto);

        pCaster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

        pCaster->SendSpellNonMeleeDamageLog(&damageInfo);

        // Set trigger flag
        uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC; //  | PROC_FLAG_SUCCESSFUL_HARMFUL_SPELL_HIT;
        uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC;// | PROC_FLAG_TAKEN_HARMFUL_SPELL_HIT;
        uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE);
        if (damageInfo.damage)
            procVictim|=PROC_FLAG_TAKEN_ANY_DAMAGE;

        pCaster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, 0, BASE_ATTACK, spellProto);

        pCaster->DealSpellDamage(&damageInfo, true);
        break;
    }
    case SPELL_AURA_MOD_REGEN:
    {
        // don't heal target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        int32 gain = target->ModifyHealth(m_modifier.m_amount);
        if (Unit *caster = GetCaster())
            target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f  * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
        break;
    }
    case SPELL_AURA_MOD_POWER_REGEN:
    {
        // don't energize target if not alive, possible death persistent effects
        if (!target->isAlive())
            return;

        Powers pt = target->getPowerType();
        if(int32(pt) != m_modifier.m_miscvalue)
            return;

        if ( spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED )
        {
            // eating anim
            target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
        }
        else if( GetId() == 20577 )
        {
            // cannibalize anim
            target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
        }

        // Anger Management
        // amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
        // so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
        if(pt == POWER_RAGE)
            target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
        break;
    }
    // Here tick dummy auras
    case SPELL_AURA_DUMMY:                              // some spells have dummy aura
    {
        PeriodicDummyTick();
        break;
    }
    case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
    {
        TriggerSpell();

        // Aura of Nature for the Emerald Dragon.
        if (GetId() == 25041)
            target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DAMAGE);

        break;
    }
    default:
        break;
    }
}

void Aura::PeriodicDummyTick()
{
    SpellEntry const* spell = GetSpellProto();
    Unit *target = GetTarget();
    switch (spell->SpellFamilyName)
    {
    case SPELLFAMILY_GENERIC:
    {
        switch (spell->Id)
        {
            // Forsaken Skills
        case 7054:
        {
            // Possibly need cast one of them (but
            // 7038 Forsaken Skill: Swords
            // 7039 Forsaken Skill: Axes
            // 7040 Forsaken Skill: Daggers
            // 7041 Forsaken Skill: Maces
            // 7042 Forsaken Skill: Staves
            // 7043 Forsaken Skill: Bows
            // 7044 Forsaken Skill: Guns
            // 7045 Forsaken Skill: 2H Axes
            // 7046 Forsaken Skill: 2H Maces
            // 7047 Forsaken Skill: 2H Swords
            // 7048 Forsaken Skill: Defense
            // 7049 Forsaken Skill: Fire
            // 7050 Forsaken Skill: Frost
            // 7051 Forsaken Skill: Holy
            // 7053 Forsaken Skill: Shadow
            return;
        }
        case 7057:                                  // Haunting Spirits
            if (roll_chance_i(33))
                target->CastSpell(target,m_modifier.m_amount,true,NULL,this);
            return;
		case 16468:                                  // Mother's Milk
            if (roll_chance_i(33))
			{
				target->CastSpell(target, 16469, true, NULL, this); // Web Explosion on the player
				target->CastSpell(target, 15474, true, NULL, this); // Web Explosion on nearby players				
			}
            return;
		case 24596:                                  // Intoxicating Venom
			int32 r = irand(0, 99);

            if (r <= 30)
				target->CastSpell(target, 6869, true, NULL, this); //Cast Fall Down on the player
			else if (r >= 70 && r <= 90)
				target->CastSpell(target, 22691, true, NULL, this); // Disarm the player		
			return;
        }
        break;
    }
    default:
        break;
    }
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if( !fearAuras.empty() )
    {
        if (apply)
            GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGuid());
        else
            GetTarget()->SetFeared(true);
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if(!Real)
        return;

    // prevent double apply bonuses
    if(apply && (GetTarget()->GetTypeId()!=TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
    {
        if(Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;
            switch(GetSpellProto()->SpellFamilyName)
            {
            case SPELLFAMILY_MAGE:
                if(GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000008000))
                {
                    // Mana Shield
                    // +50% from +spd bonus
                    DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto())) * 0.5f;
                    break;
                }
                break;
            default:
                break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

bool Aura::IsLastAuraOnHolder()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (i != GetEffIndex() && GetHolder()->m_auras[i])
            return false;
    return true;
}

SpellAuraHolder::SpellAuraHolder(SpellEntry const* spellproto, bool isTriggered, Unit *target, WorldObject *caster, Item *castItem) :
    m_target(target), m_castItemGuid(castItem ? castItem->GetObjectGuid() : ObjectGuid()),
    m_spellProto(spellproto), m_isTriggered(isTriggered), m_auraSlot(MAX_AURAS), m_auraLevel(1), m_procCharges(0),
    m_stackAmount(1), m_timeCla(1000), m_removeMode(AURA_REMOVE_BY_DEFAULT),
    m_AuraDRGroup(DIMINISHING_NONE), m_permanent(false), m_isRemovedOnShapeLost(true), m_deleted(false), m_in_use(0)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    if(!caster)
        m_casterGuid = target->GetObjectGuid();
    else
    {
        // remove this assert when not unit casters will be supported
        MANGOS_ASSERT(caster->isType(TYPEMASK_UNIT))
        m_casterGuid = caster->GetObjectGuid();
    }

    m_applyTime      = time(NULL);
    m_isPassive      = IsPassiveSpell(spellproto);
    m_isDeathPersist = IsDeathPersistentSpell(spellproto);
    m_isSingleTarget = IsSingleTargetSpell(spellproto);
    m_procCharges    = spellproto->procCharges;

    m_isRemovedOnShapeLost = (GetCasterGuid() == m_target->GetObjectGuid() &&
                              m_spellProto->Stances &&
                              !(m_spellProto->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) &&
                              !(m_spellProto->Attributes & SPELL_ATTR_NOT_SHAPESHIFT));

    Unit* unitCaster = caster && caster->isType(TYPEMASK_UNIT) ? (Unit*)caster : NULL;

    m_duration = m_maxDuration = CalculateSpellDuration(spellproto, unitCaster);

    if (m_maxDuration == -1 || (m_isPassive && spellproto->DurationIndex == 0))
        m_permanent = true;

    if (unitCaster)
    {
        if (Player* modOwner = unitCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
    }

    // some custom stack values at aura holder create
    switch (m_spellProto->Id)
    {
        // some auras applied with max stack
    case 24575:                                         // Brittle Armor
    case 24659:                                         // Unstable Power
    case 24662:                                         // Restless Strength
    case 26464:                                         // Mercurial Shield
        m_stackAmount = m_spellProto->StackAmount;
        break;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        m_auras[i] = NULL;
}

void SpellAuraHolder::AddAura(Aura *aura, SpellEffectIndex index)
{
    m_auras[index] = aura;
}

void SpellAuraHolder::RemoveAura(SpellEffectIndex index)
{
    m_auras[index] = NULL;
}

void SpellAuraHolder::ApplyAuraModifiers(bool apply, bool real)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX && !IsDeleted(); ++i)
        if (Aura *aur = GetAuraByEffectIndex(SpellEffectIndex(i)))
            aur->ApplyModifier(apply, real);
}

void SpellAuraHolder::_AddSpellAuraHolder()
{
    if (!GetId())
        return;
    if(!m_target)
        return;

    // Try find slot for aura
    uint8 slot = NULL_AURA_SLOT;
    Unit* caster = GetCaster();

    // Lookup free slot
    // will be < MAX_AURAS slot (if find free) with !secondaura
    if (IsNeedVisibleSlot(caster))
    {
        if (IsPositive())                                   // empty positive slot
        {
            for (uint8 i = 0; i < MAX_POSITIVE_AURAS; i++)
            {
                if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                {
                    slot = i;
                    break;
                }
            }
        }
        else                                                // empty negative slot
        {
            uint8 bestAuraSlot = NULL_AURA_SLOT;
            SpellAuraHolder *bestAuraHolder = NULL;
            AuraPriority bestAuraPriority = AURA_PRIORITY_MOST;

            if (!this->IsTriggered() && IsBinarySpell(this->GetSpellProto()))
            {
                bestAuraPriority = AURA_PRIORITY_BINARY_NONPROC;
            }

            for (uint8 i = MAX_POSITIVE_AURAS; i < MAX_AURAS; i++)
            {
                AuraPriority currentAuraPriority = AURA_PRIORITY_MOST;
                uint32 currentId = m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i));
                SpellAuraHolder *currentAuraHolder = NULL;
                if (currentId == 0)
                {
                    currentAuraPriority = AURA_PRIORITY_EMPTY;
                } else
                {
                    Unit::SpellAuraHolderMap::const_iterator iter = m_target->GetSpellAuraHolderMap().find(currentId);
                    currentAuraHolder = (*iter).second;

                    currentAuraPriority = AURA_PRIORITY_MOST;

                    if (!this->IsTriggered() && IsBinarySpell(this->GetSpellProto()))
                        currentAuraPriority = AURA_PRIORITY_BINARY_NONPROC;
                }

                if (currentAuraPriority < bestAuraPriority || (currentAuraPriority == bestAuraPriority && bestAuraHolder != NULL && currentAuraHolder != NULL && currentAuraHolder->GetAuraApplyTime() < bestAuraHolder->GetAuraApplyTime()))
                {
                    bestAuraPriority = currentAuraPriority;
                    bestAuraSlot = i;
                    bestAuraHolder = currentAuraHolder;
                }
            }

            if (bestAuraSlot == NULL_AURA_SLOT)
                return;
            else
            {
                slot = bestAuraSlot;

                if (bestAuraHolder != NULL)
                {
                    m_target->RemoveSpellAuraHolder(bestAuraHolder);
                }
            }
        }
    }

    // set infinity cooldown state for spells
    if(caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = m_castItemGuid ? ((Player*)caster)->GetItemByGuid(m_castItemGuid) : NULL;
            ((Player*)caster)->AddSpellAndCategoryCooldowns(m_spellProto,castItem ? castItem->GetEntry() : 0, NULL,true);
        }
    }

    SetAuraSlot(slot);

    // Not update fields for not first spell's aura, all data already in fields
    if (slot < MAX_AURAS)                                   // slot found
    {
        SetAura(slot, false);
        SetAuraFlag(slot, true);
        SetAuraLevel(slot,caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
        UpdateAuraApplication();

        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        UpdateAuraDuration();
    }

    //*****************************************************
    // Update target aura state flag (at 1 aura apply)
    // TODO: Make it easer
    //*****************************************************
    // Sitdown on apply aura req seated
    if (m_spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !m_target->IsSitState())
        m_target->SetStandState(UNIT_STAND_STATE_SIT);

    // register aura diminishing on apply
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(),true);

    // Update Seals information
    if (IsSealSpell(GetSpellProto()))
        m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);
}

void SpellAuraHolder::_RemoveSpellAuraHolder()
{
    // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if(m_removeMode!=AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if(caster && IsPersistent())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId());
        if (dynObj)
            dynObj->RemoveAffected(m_target);
    }

    //passive auras do not get put in slots
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return false;

    uint8 slot = GetAuraSlot();

    if(slot >= MAX_AURAS)                                   // slot not set
        return;

    if(m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + slot)) == 0)
        return;

    // unregister aura diminishing (and store last time)
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(), false);

    SetAura(slot, true);
    SetAuraFlag(slot, false);
    SetAuraLevel(slot,caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    m_procCharges = 0;
    m_stackAmount = 1;
    UpdateAuraApplication();

    if (m_removeMode != AURA_REMOVE_BY_DELETE)
    {
        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        //*****************************************************
        // Update target aura state flag (at last aura remove)
        //*****************************************************
        uint32 removeState = 0;
        ClassFamilyMask removeFamilyFlag = m_spellProto->SpellFamilyFlags;
        switch(m_spellProto->SpellFamilyName)
        {
        case SPELLFAMILY_PALADIN:
            if (IsSealSpell(m_spellProto))
                removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
            break;
        }

        // Remove state (but need check other auras for it)
        if (removeState)
        {
            bool found = false;
            Unit::SpellAuraHolderMap const& holders = m_target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator i = holders.begin(); i != holders.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if (auraSpellInfo->IsFitToFamily(SpellFamily(m_spellProto->SpellFamilyName), removeFamilyFlag))
                {
                    found = true;
                    break;
                }
            }

            // this has been last aura
            if(!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if(caster && caster->GetTypeId() == TYPEID_PLAYER)
        {
            if ( GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE )
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                ((Player*)caster)->SendCooldownEvent(GetSpellProto());
        }
    }
}

void SpellAuraHolder::CleanupTriggeredSpells()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 tSpellId = m_spellProto->EffectTriggerSpell[i];
        if(!tSpellId)
            continue;

        SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
        if(!tProto)
            continue;

        if(GetSpellDuration(tProto) != -1)
            continue;

        // needed for spell 43680, maybe others
        // TODO: is there a spell flag, which can solve this in a more sophisticated way?
        if (m_spellProto->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
                GetSpellDuration(m_spellProto) == int32(m_spellProto->EffectAmplitude[i]))
            continue;

        m_target->RemoveAurasDueToSpell(tSpellId);
    }
}

bool SpellAuraHolder::ModStackAmount(int32 num)
{
    uint32 protoStackAmount = m_spellProto->StackAmount;

    // Can`t mod
    if (!protoStackAmount)
        return true;

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > (int32)protoStackAmount)
        stackAmount = protoStackAmount;
    else if (stackAmount <=0) // Last aura from stack removed
    {
        m_stackAmount = 0;
        return true; // need remove aura
    }

    // Update stack amount
    SetStackAmount(stackAmount);
    return false;
}

void SpellAuraHolder::SetStackAmount(uint32 stackAmount)
{
    Unit *target = GetTarget();
    Unit *caster = GetCaster();
    if (!target || !caster)
        return;

    bool refresh = stackAmount >= m_stackAmount;
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;
        UpdateAuraApplication();

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aur = m_auras[i])
            {
                int32 bp = aur->GetBasePoints();
                int32 amount = m_stackAmount * caster->CalculateSpellDamage(target, m_spellProto, SpellEffectIndex(i), &bp);
                // Reapply if amount change
                if (amount != aur->GetModifier()->m_amount)
                {
                    aur->ApplyModifier(false, true);
                    aur->GetModifier()->m_amount = amount;
                    aur->ApplyModifier(true, true);
                }
            }
        }
    }

    if (refresh)
        // Stack increased refresh duration
        RefreshHolder();
}

Unit* SpellAuraHolder::GetCaster() const
{
    if(GetCasterGuid() == m_target->GetObjectGuid())
        return m_target;

    return ObjectAccessor::GetUnit(*m_target, m_casterGuid);// player will search at any maps
}

bool SpellAuraHolder::IsWeaponBuffCoexistableWith(SpellAuraHolder const* ref) const
{
    // only item casted spells
    if (!GetCastItemGuid())
        return false;

    // Exclude Debuffs
    if (!IsPositive())
        return false;

    // Exclude Non-generic Buffs and Executioner-Enchant
    if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC)
        return false;

    // Exclude Stackable Buffs [ie: Blood Reserve]
    if (GetSpellProto()->StackAmount)
        return false;

    // only self applied player buffs
    if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetObjectGuid() != GetCasterGuid())
        return false;

    Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGuid());
    if (!castItem)
        return false;

    // Limit to Weapon-Slots
    if (!castItem->IsEquipped() ||
            (castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
        return false;

    // form different weapons
    return ref->GetCastItemGuid() && ref->GetCastItemGuid() != GetCastItemGuid();
}

bool SpellAuraHolder::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem();

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_auras[i])
            continue;

        // special area auras cases
        switch(m_spellProto->Effect[i])
        {
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            // passive auras (except totem auras) do not get placed in caster slot
            return (m_target != caster || totemAura || !m_isPassive) && m_auras[i]->GetModifier()->m_auraname != SPELL_AURA_NONE;
        default:
            break;
        }
    }

    // passive auras (except totem auras) do not get placed in the slots
    return !m_isPassive || totemAura;
}

void SpellAuraHolder::HandleSpellSpecificBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 spellId4 = 0;

    switch(GetSpellProto()->SpellFamilyName)
    {
    case SPELLFAMILY_MAGE:
    {
        switch(GetId())
        {
        case 11189:                                 // Frost Warding
        case 28332:
        {
            if(m_target->GetTypeId()==TYPEID_PLAYER && !apply)
            {
                // reflection chance (effect 1) of Frost Ward, applied in dummy effect
                if (SpellModifier *mod = ((Player*)m_target)->GetSpellMod(SPELLMOD_EFFECT2, GetId()))
                    ((Player*)m_target)->AddSpellMod(mod, false);
            }
            return;
        }
        default:
            return;
        }
        break;
    }
    case SPELLFAMILY_HUNTER:
    {
        switch(GetId())
        {
            // The Beast Within and Bestial Wrath - immunity
        case 19574:
        {
            spellId1 = 24395;
            spellId2 = 24396;
            spellId3 = 24397;
            spellId4 = 26592;
            break;
        }
        default:
            return;
        }
        break;
    }
    default:
        return;
    }

    // prevent aura deletion, specially in multi-boost case
    SetInUse(true);

    if (apply)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, NULL, NULL, GetCasterGuid());
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, NULL, NULL, GetCasterGuid());
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, NULL, NULL, GetCasterGuid());
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, NULL, NULL, GetCasterGuid());
    }
    else
    {
        if (spellId1)
            m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGuid());
        if (spellId2)
            m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGuid());
        if (spellId3)
            m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGuid());
        if (spellId4)
            m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGuid());
    }

    SetInUse(false);
}

void Aura::HandleAuraSafeFall( bool Apply, bool Real )
{
    // implemented in WorldSession::HandleMovementOpcodes

    // only special case
    if(Apply && Real && GetId() == 32474 && GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->ActivateTaxiPathTo(506, GetId());
}

SpellAuraHolder::~SpellAuraHolder()
{
    // note: auras in delete list won't be affected since they clear themselves from holder when adding to deletedAuraslist
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            delete aur;
}

void SpellAuraHolder::Update(uint32 diff)
{
    //Periodically tick- when we tick check for periodic power costs
    //We only do this for the "first" aura on a spell, since maybe we have a few
    //aura holders and we don't want them to be all ticking mana costs
    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;

        //Find the first aura on the associated spell and see if it's being run off
        //this aura holder
        bool isPrimeHolder = true;
        for (SpellEffectIndex effectIndex = EFFECT_INDEX_0; effectIndex <= MAX_EFFECT_INDEX; effectIndex = (SpellEffectIndex)(1 + (uint32)effectIndex))
        {
            if (this->GetSpellProto()->Effect[effectIndex] == SPELL_EFFECT_APPLY_AURA)
            {
                if (this->GetAuraByEffectIndex(effectIndex) == NULL)
                    isPrimeHolder = false;
                break;
            }
        }

        if (isPrimeHolder)
        {
            m_timeCla -= diff;

            if (m_timeCla <= 0)
            {
                if (Unit* caster = GetCaster())
                {
                    Powers powertype = Powers(GetSpellProto()->powerType);
                    int32 spellCost = GetSpellProto()->manaPerSecond + GetSpellProto()->manaPerSecondPerLevel * caster->getLevel();
                    m_timeCla = 1*IN_MILLISECONDS;

                    if (spellCost != 0)
                    {
                        // health as power used
                        if(powertype == POWER_HEALTH)
                        {
                            //Do damage- we allow ourselves to die in these cases because it was the style at the time
                            caster->DealDamage(caster,spellCost,NULL,SELF_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,NULL,false);

                            //Don't continue processing if dead
                            if (!caster->isAlive())
                                return;
                        } else if(powertype >= MAX_POWERS)
                        {
                            caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                            sLog.outError("Spell::TakePower: Unknown power type '%d'", powertype);
                            return;
                        } else if (caster->GetPower(powertype) < (uint32)spellCost)
                        {
                            //No power- stop doing this stuff
                            caster->RemoveAurasDueToSpell(GetId());
                            caster->FinishSpell(CURRENT_GENERIC_SPELL);
                            caster->FinishSpell(CURRENT_CHANNELED_SPELL);
                            return;
                        } else
                        {
                            caster->ModifyPower(powertype, -spellCost);

                            // Set the five second timer
                            if (powertype == POWER_MANA && spellCost > 0)
                                caster->SetLastManaUse();
                        }
                    }
                }
            }
        }
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aura = m_auras[i])
            aura->UpdateAura(diff);

    // Channeled aura required check distance from caster
    if(IsChanneledSpell(m_spellProto) && GetCasterGuid() != m_target->GetObjectGuid())
    {
        Unit* caster = GetCaster();
        if(!caster)
        {
            m_target->RemoveAurasByCasterSpell(GetId(), GetCasterGuid());
            return;
        }

        // need check distance for channeled target only
        if (caster->GetChannelObjectGuid() == m_target->GetObjectGuid())
        {
            // Get spell range
            float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

            if(Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, NULL);

            //This is no strict range check: add 6.25, just like we do it in Spell::CheckRange!
            max_range += 6.25;

            if(!caster->IsWithinDistInMap(m_target, max_range))
            {
                caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
                return;
            }
        }
    }
}

void SpellAuraHolder::RefreshHolder()
{
    SetAuraDuration(GetAuraMaxDuration());
    UpdateAuraDuration();
}

void SpellAuraHolder::SetAuraMaxDuration(int32 duration)
{
    m_maxDuration = duration;

    // possible overwrite persistent state
    if (duration > 0)
    {
        if (!(IsPassive() && GetSpellProto()->DurationIndex == 0))
            SetPermanent(false);
    }
}

bool SpellAuraHolder::HasMechanic(uint32 mechanic) const
{
    if (mechanic == m_spellProto->Mechanic)
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] == mechanic)
            return true;
    return false;
}

bool SpellAuraHolder::HasMechanicMask(uint32 mechanicMask) const
{
    if (mechanicMask & (1 << (m_spellProto->Mechanic - 1)))
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] && ((1 << (m_spellProto->EffectMechanic[i] -1)) & mechanicMask))
            return true;
    return false;
}

bool SpellAuraHolder::IsPersistent() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsPersistent())
                return true;
    return false;
}

bool SpellAuraHolder::IsAreaAura() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsAreaAura())
                return true;
    return false;
}

bool SpellAuraHolder::IsPositive() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (!aur->IsPositive())
                return false;
    return true;
}

bool SpellAuraHolder::IsEmptyHolder() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i])
            return false;
    return true;
}

void SpellAuraHolder::UnregisterSingleCastHolder()
{
    if (IsSingleTarget())
    {
        if(Unit* caster = GetCaster())
            caster->GetSingleCastSpellTargets().erase(GetSpellProto());

        m_isSingleTarget = false;
    }
}

void SpellAuraHolder::SetAuraFlag(uint32 slot, bool add)
{
    uint32 index    = slot >> 3;
    uint32 byte     = (slot & 7) << 2;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
    if (add)
        val |= ((uint32)AFLAG_MASK << byte);
    else
        val &= ~((uint32)AFLAG_MASK << byte);

    m_target->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void SpellAuraHolder::SetAuraLevel(uint32 slot,uint32 level)
{
    uint32 index    = slot / 4;
    uint32 byte     = (slot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
    val &= ~(0xFF << byte);
    val |= (level << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void SpellAuraHolder::UpdateAuraApplication()
{
    if (m_auraSlot >= MAX_AURAS)
        return;

    uint32 stackCount = m_procCharges > 0 ? m_procCharges*m_stackAmount : m_stackAmount;

    uint32 index    = m_auraSlot / 4;
    uint32 byte     = (m_auraSlot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
    val &= ~(0xFF << byte);
    // field expect count-1 for proper amount show, also prevent overflow at client side
    val |= ((uint8(stackCount <= 255 ? stackCount-1 : 255-1)) << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void SpellAuraHolder::UpdateAuraDuration()
{
    if(GetAuraSlot() >= MAX_AURAS || m_isPassive)
        return;

    if( m_target->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_UPDATE_AURA_DURATION, 5);
        data << uint8(GetAuraSlot());
        data << uint32(GetAuraDuration());
        ((Player*)m_target)->SendDirectMessage(&data);

        //data.Initialize(SMSG_SET_EXTRA_AURA_INFO, (8+1+4+4+4));
        //data << m_target->GetPackGUID();
        //data << uint8(GetAuraSlot());
        //data << uint32(GetId());
        //data << uint32(GetAuraMaxDuration());
        //data << uint32(GetAuraDuration());
        //((Player*)m_target)->SendDirectMessage(&data);
    }

    // not send in case player loading (will not work anyway until player not added to map), sent in visibility change code
    if(m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading())
        return;

    Unit* caster = GetCaster();

    if(caster && caster->GetTypeId() == TYPEID_PLAYER && caster != m_target)
        SendAuraDurationForCaster((Player*)caster);
}

void SpellAuraHolder::SendAuraDurationForCaster(Player* caster)
{
    WorldPacket data(SMSG_SET_EXTRA_AURA_INFO_NEED_UPDATE, (8+1+4+4+4));
    data << m_target->GetPackGUID();
    data << uint8(GetAuraSlot());
    data << uint32(GetId());
    data << uint32(GetAuraMaxDuration());                   // full
    data << uint32(GetAuraDuration());                      // remain
    caster->GetSession()->SendPacket(&data);
}
