/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "World.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "BattlegroundMap.h"
#include "BattlegroundAB.h"
#include "Creature.h"
#include "Language.h"
#include "Object.h"
#include "Player.h"
#include "Util.h"

// these variables aren't used outside of this file, so declare them only here
uint32 BG_AB_HonorScoreTicks[BG_HONOR_MODE_NUM] =
{
    330, // normal honor
    200  // holiday
};

uint32 BG_AB_ReputationScoreTicks[BG_HONOR_MODE_NUM] =
{
    200, // normal honor
    150  // holiday
};

void BattlegroundAB::InstallBattleground()
{
    TeamScores[BG_TEAM_ALLIANCE]          = 0;
    TeamScores[BG_TEAM_HORDE]             = 0;
    _lastTick[BG_TEAM_ALLIANCE]            = 0;
    _lastTick[BG_TEAM_HORDE]               = 0;
    _honorScoreTicks[BG_TEAM_ALLIANCE]      = 0;
    _honorScoreTicks[BG_TEAM_HORDE]         = 0;
    _reputationScoreTicks[BG_TEAM_ALLIANCE] = 0;
    _reputationScoreTicks[BG_TEAM_HORDE]    = 0;
    _isInformedNearVictory                 = false;
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    _honorTicks = (isBGWeekend) ? BG_AB_ABBGWeekendHonorTicks : BG_AB_NotABBGWeekendHonorTicks;
    _reputationTicks = (isBGWeekend) ? BG_AB_ABBGWeekendReputationTicks : BG_AB_NotABBGWeekendReputationTicks;
    _teamScores500Disadvantage[BG_TEAM_ALLIANCE] = false;
    _teamScores500Disadvantage[BG_TEAM_HORDE]    = false;

    for (uint8 i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
    {
        _nodes[i] = 0;
        _prevNodes[i] = 0;
        _nodeTimers[i] = 0;
        _bannerTimers[i].timer = 0;
    }
}

void BattlegroundAB::InitializeObjects()
{
    ObjectGUIDsByType.resize(BG_AB_OBJECT_MAX + BG_AB_ALL_NODES_COUNT + 5);

    for (uint8 i = 0 ; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
    {
        AddGameObject(BG_AB_OBJECT_BANNER_NEUTRAL + 8*i, BG_AB_OBJECTID_NODE_BANNER_0 + i, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_BANNER_CONT_A + 8*i, BG_AB_OBJECTID_BANNER_CONT_A, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_BANNER_CONT_H + 8*i, BG_AB_OBJECTID_BANNER_CONT_H, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_BANNER_ALLY + 8*i, BG_AB_OBJECTID_BANNER_A, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_BANNER_HORDE + 8*i, BG_AB_OBJECTID_BANNER_H, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_AURA_ALLY + 8*i, BG_AB_OBJECTID_AURA_A, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_AURA_HORDE + 8*i, BG_AB_OBJECTID_AURA_H, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_AURA_CONTESTED + 8*i, BG_AB_OBJECTID_AURA_C, BG_AB_NodePositions[i][0], BG_AB_NodePositions[i][1], BG_AB_NodePositions[i][2], BG_AB_NodePositions[i][3], 0, 0, sin(BG_AB_NodePositions[i][3]/2), cos(BG_AB_NodePositions[i][3]/2), RESPAWN_ONE_DAY);
    }

    AddGameObject(BG_AB_OBJECT_GATE_A, BG_AB_OBJECTID_GATE_A, BG_AB_DoorPositions[0][0], BG_AB_DoorPositions[0][1], BG_AB_DoorPositions[0][2], BG_AB_DoorPositions[0][3], BG_AB_DoorPositions[0][4], BG_AB_DoorPositions[0][5], BG_AB_DoorPositions[0][6], BG_AB_DoorPositions[0][7], RESPAWN_IMMEDIATELY);
    AddGameObject(BG_AB_OBJECT_GATE_H, BG_AB_OBJECTID_GATE_H, BG_AB_DoorPositions[1][0], BG_AB_DoorPositions[1][1], BG_AB_DoorPositions[1][2], BG_AB_DoorPositions[1][3], BG_AB_DoorPositions[1][4], BG_AB_DoorPositions[1][5], BG_AB_DoorPositions[1][6], BG_AB_DoorPositions[1][7], RESPAWN_IMMEDIATELY);

    //buffs
    for (uint8 i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
    {
        AddGameObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i, BuffEntries[0], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i + 1, Buff_Entries[1], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY);
        AddGameObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i + 2, BuffEntries[2], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY);
    }

    // despawn banners, auras and buffs
    for (int obj = BG_AB_OBJECT_BANNER_NEUTRAL; obj < BG_AB_DYNAMIC_NODES_COUNT * 8; ++obj)
        SpawnGameObject(obj, RESPAWN_ONE_DAY);
    for (int i = 0; i < BG_AB_DYNAMIC_NODES_COUNT * 3; ++i)
        SpawnGameObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + i, RESPAWN_ONE_DAY);

    // Starting doors
    DoorClose(BG_AB_OBJECT_GATE_A);
    DoorClose(BG_AB_OBJECT_GATE_H);
    SpawnGameObject(BG_AB_OBJECT_GATE_A, RESPAWN_IMMEDIATELY);
    SpawnGameObject(BG_AB_OBJECT_GATE_H, RESPAWN_IMMEDIATELY);

    // Starting base spirit guides
    _NodeOccupied(BG_AB_SPIRIT_ALIANCE, ALLIANCE);
    _NodeOccupied(BG_AB_SPIRIT_HORDE, HORDE);
}

void BattlegroundAB::InitializeTextIds()
{
    PreparationPhaseTextIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_AB_START_TWO_MINUTES;
    PreparationPhaseTextIds[BG_STARTING_EVENT_SECOND] = LANG_BG_AB_START_ONE_MINUTE;
    PreparationPhaseTextIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_AB_START_HALF_MINUTE;
    PreparationPhaseTextIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_AB_HAS_BEGUN;    
}

void BattlegroundAB::ProcessInProgress(uint32 const& diff)
{
    BattlegroundMap::ProcessInProgress(diff);

    int team_points[BG_TEAMS_COUNT] = { 0, 0 };

    for (int node = 0; node < BG_AB_DYNAMIC_NODES_COUNT; ++node)
    {
        // 3 sec delay to spawn new banner instead previous despawned one
        if (_bannerTimers[node].timer)
        {
            if (_bannerTimers[node].timer > diff)
                _bannerTimers[node].timer -= diff;
            else
            {
                _bannerTimers[node].timer = 0;
                _CreateBanner(node, _bannerTimers[node].type, _bannerTimers[node].teamIndex, false);
            }
        }

        // 1-minute to occupy a node from contested state
        if (_nodeTimers[node])
        {
            if (_nodeTimers[node] > diff)
                _nodeTimers[node] -= diff;
            else
            {
                _nodeTimers[node] = 0;
                // Change from contested to occupied !
                uint8 teamIndex = _nodes[node]-1;
                _prevNodes[node] = _nodes[node];
                _nodes[node] += 2;
                // burn current contested banner
                _DelBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex);
                // create new occupied banner
                _CreateBanner(node, BG_AB_NODE_TYPE_OCCUPIED, teamIndex, true);
                _SendNodeUpdate(node);
                _NodeOccupied(node, (teamIndex == 0) ? ALLIANCE:HORDE);
                // Message to chatlog

                if (teamIndex == 0)
                {
                    // FIXME: team and node names not localized
                    SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_TAKEN, LANG_BG_AB_ALLY, _GetNodeNameId(node)), CHAT_MSG_BG_SYSTEM_ALLIANCE);
                    PlaySoundToAll(BG_AB_SOUND_NODE_CAPTURED_ALLIANCE);
                }
                else
                {
                    // FIXME: team and node names not localized
                    SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_TAKEN, LANG_BG_AB_HORDE, _GetNodeNameId(node)), CHAT_MSG_BG_SYSTEM_HORDE);
                    PlaySoundToAll(BG_AB_SOUND_NODE_CAPTURED_HORDE);
                }
            }
        }

        for (int team = 0; team < BG_TEAMS_COUNT; ++team)
            if (_nodes[node] == team + BG_AB_NODE_TYPE_OCCUPIED)
                ++team_points[team];
    }

    // Accumulate points
    for (int team = 0; team < BG_TEAMS_COUNT; ++team)
    {
        int points = team_points[team];
        if (!points)
            continue;
        _lastTick[team] += diff;
        if (_lastTick[team] > BG_AB_TickIntervals[points])
        {
            _lastTick[team] -= BG_AB_TickIntervals[points];
            TeamScores[team] += BG_AB_TickPoints[points];
            _honorScoreTicks[team] += BG_AB_TickPoints[points];
            _reputationScoreTicks[team] += BG_AB_TickPoints[points];
            if (_reputationScoreTicks[team] >= _reputationTicks)
            {
                (team == BG_TEAM_ALLIANCE) ? RewardReputationToTeam(509, 10, ALLIANCE) : RewardReputationToTeam(510, 10, HORDE);
                _reputationScoreTicks[team] -= _reputationTicks;
            }
            if (_honorScoreTicks[team] >= _honorTicks)
            {
                RewardHonorToTeam(GetBonusHonorFromKill(1), team);
                _honorScoreTicks[team] -= _honorTicks;
            }
            if (!_isInformedNearVictory && TeamScores[team] > BG_AB_WARNING_NEAR_VICTORY_SCORE)
            {
                if (team == BG_TEAM_ALLIANCE)
                    SendMessageToAll(LANG_BG_AB_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                else
                    SendMessageToAll(LANG_BG_AB_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                PlaySoundToAll(BG_AB_SOUND_NEAR_VICTORY);
                _isInformedNearVictory = true;
            }

            if (TeamScores[team] > BG_AB_MAX_TEAM_SCORE)
                TeamScores[team] = BG_AB_MAX_TEAM_SCORE;
            if (team == BG_TEAM_ALLIANCE)
                UpdateWorldState(BG_AB_OP_RESOURCES_ALLY, TeamScores[team]);
            if (team == BG_TEAM_HORDE)
                UpdateWorldState(BG_AB_OP_RESOURCES_HORDE, TeamScores[team]);
            // update achievement flags
            // we increased TeamScores[team] so we just need to check if it is 500 more than other teams resources
            uint8 otherTeam = (team + 1) % BG_TEAMS_COUNT;
            if (TeamScores[team] > TeamScores[otherTeam] + 500)
                _teamScores500Disadvantage[otherTeam] = true;
        }
    }

    // Test win condition
    if (TeamScores[BG_TEAM_ALLIANCE] >= BG_AB_MAX_TEAM_SCORE)
        EndBattleground(WINNER_ALLIANCE);
    if (TeamScores[BG_TEAM_HORDE] >= BG_AB_MAX_TEAM_SCORE)
        EndBattleground(WINNER_HORDE);
}

void BattlegroundAB::OnPlayerJoin(Player *plr)
{
    BattlegroundMap::OnPlayerJoin(plr);
    //create score and add it to map, default values are set in the constructor
    BattlegroundABScore* sc = new BattlegroundABScore;

    PlayerScores[plr->GetGUIDLow()] = sc;
}

void BattlegroundAB::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch(Trigger)
    {
        case 3948:                                          // Arathi Basin Alliance Exit.
            if (Source->GetTeam() != ALLIANCE)
                Source->GetSession()->SendAreaTriggerMessage("Only The Alliance can use that portal");
            else
                Source->LeaveBattleground();
            break;
        case 3949:                                          // Arathi Basin Horde Exit.
            if (Source->GetTeam() != HORDE)
                Source->GetSession()->SendAreaTriggerMessage("Only The Horde can use that portal");
            else
                Source->LeaveBattleground();
            break;
        case 3866:                                          // Stables
        case 3869:                                          // Gold Mine
        case 3867:                                          // Farm
        case 3868:                                          // Lumber Mill
        case 3870:                                          // Black Smith
        case 4020:                                          // Unk1
        case 4021:                                          // Unk2
            //break;
        default:
            //sLog->outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            //Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

/*  type: 0-neutral, 1-contested, 3-occupied
    teamIndex: 0-ally, 1-horde                        */
void BattlegroundAB::_CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
{
    // Just put it into the queue
    if (delay)
    {
        _bannerTimers[node].timer = 2000;
        _bannerTimers[node].type = type;
        _bannerTimers[node].teamIndex = teamIndex;
        return;
    }

    uint8 obj = node*8 + type + teamIndex;

    SpawnGameObject(obj, RESPAWN_IMMEDIATELY);

    // handle aura with banner
    if (!type)
        return;
    obj = node * 8 + ((type == BG_AB_NODE_TYPE_OCCUPIED) ? (5 + teamIndex) : 7);
    SpawnGameObject(obj, RESPAWN_IMMEDIATELY);
}

void BattlegroundAB::_DelBanner(uint8 node, uint8 type, uint8 teamIndex)
{
    uint8 obj = node*8 + type + teamIndex;
    SpawnGameObject(obj, RESPAWN_ONE_DAY);

    // handle aura with banner
    if (!type)
        return;
    obj = node * 8 + ((type == BG_AB_NODE_TYPE_OCCUPIED) ? (5 + teamIndex) : 7);
    SpawnGameObject(obj, RESPAWN_ONE_DAY);
}

int32 BattlegroundAB::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_AB_NODE_STABLES:    return LANG_BG_AB_NODE_STABLES;
        case BG_AB_NODE_BLACKSMITH: return LANG_BG_AB_NODE_BLACKSMITH;
        case BG_AB_NODE_FARM:       return LANG_BG_AB_NODE_FARM;
        case BG_AB_NODE_LUMBER_MILL:return LANG_BG_AB_NODE_LUMBER_MILL;
        case BG_AB_NODE_GOLD_MINE:  return LANG_BG_AB_NODE_GOLD_MINE;
        default:
            ASSERT(0);
    }
    return 0;
}

void BattlegroundAB::FillInitialWorldStates(WorldPacket& data)
{
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    // Node icons
    for (uint8 node = 0; node < BG_AB_DYNAMIC_NODES_COUNT; ++node)
        data << uint32(BG_AB_OP_NODEICONS[node]) << uint32((_nodes[node] == 0)?1:0);

    // Node occupied states
    for (uint8 node = 0; node < BG_AB_DYNAMIC_NODES_COUNT; ++node)
        for (uint8 i = 1; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
            data << uint32(BG_AB_OP_NODESTATES[node] + plusArray[i]) << uint32((_nodes[node] == i)?1:0);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 node = 0; node < BG_AB_DYNAMIC_NODES_COUNT; ++node)
        if (_nodes[node] == BG_AB_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (_nodes[node] == BG_AB_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    data << uint32(BG_AB_OP_OCCUPIED_BASES_ALLY)  << uint32(ally);
    data << uint32(BG_AB_OP_OCCUPIED_BASES_HORDE) << uint32(horde);

    // Team scores
    data << uint32(BG_AB_OP_RESOURCES_MAX)      << uint32(BG_AB_MAX_TEAM_SCORE);
    data << uint32(BG_AB_OP_RESOURCES_WARNING)  << uint32(BG_AB_WARNING_NEAR_VICTORY_SCORE);
    data << uint32(BG_AB_OP_RESOURCES_ALLY)     << uint32(TeamScores[BG_TEAM_ALLIANCE]);
    data << uint32(BG_AB_OP_RESOURCES_HORDE)    << uint32(TeamScores[BG_TEAM_HORDE]);

    // other unknown
    data << uint32(0x745) << uint32(0x2);           // 37 1861 unk
}

void BattlegroundAB::_SendNodeUpdate(uint8 node)
{
    // Send node owner state update to refresh map icons on client
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    if (_prevNodes[node])
        UpdateWorldState(BG_AB_OP_NODESTATES[node] + plusArray[_prevNodes[node]], 0);
    else
        UpdateWorldState(BG_AB_OP_NODEICONS[node], 0);

    UpdateWorldState(BG_AB_OP_NODESTATES[node] + plusArray[_nodes[node]], 1);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
        if (_nodes[i] == BG_AB_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (_nodes[i] == BG_AB_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    UpdateWorldState(BG_AB_OP_OCCUPIED_BASES_ALLY, ally);
    UpdateWorldState(BG_AB_OP_OCCUPIED_BASES_HORDE, horde);
}

void BattlegroundAB::_NodeOccupied(uint8 node, Team team)
{
    if (!AddSpiritGuide(node, BG_AB_SpiritGuidePos[node][0], BG_AB_SpiritGuidePos[node][1], BG_AB_SpiritGuidePos[node][2], BG_AB_SpiritGuidePos[node][3], team))
        sLog->outError("Failed to spawn spirit guide! point: %u, team: %u, ", node, team);

    uint8 capturedNodes = 0;
    for (uint8 i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
    {
        if (_nodes[node] == GetTeamIndexByTeamId(team) + BG_AB_NODE_TYPE_OCCUPIED && !_nodeTimers[i])
            ++capturedNodes;
    }
    if (capturedNodes >= 5)
        CastSpellOnTeam(SPELL_AB_QUEST_REWARD_5_BASES, team);
    if (capturedNodes >= 4)
        CastSpellOnTeam(SPELL_AB_QUEST_REWARD_4_BASES, team);

    if (node >= BG_AB_DYNAMIC_NODES_COUNT)//only dynamic nodes, no start points
        return;
    Creature* trigger = GetCreature(node+7);//0-6 spirit guides
    if (!trigger)
       trigger = AddCreature(WORLD_TRIGGER, node+7, team, BG_AB_NodePositions[node][0], BG_AB_NodePositions[node][1], BG_AB_NodePositions[node][2], BG_AB_NodePositions[node][3]);

    //add bonus honor aura trigger creature when node is accupied
    //cast bonus aura (+50% honor in 25yards)
    //aura should only apply to players who have accupied the node, set correct faction for trigger
    if (trigger)
    {
        trigger->setFaction(team == ALLIANCE ? 84 : 83);
        trigger->CastSpell(trigger, SPELL_HONORABLE_DEFENDER_25Y, false);
    }
}

void BattlegroundAB::_NodeDeOccupied(uint8 node)
{
    if (node >= BG_AB_DYNAMIC_NODES_COUNT)
        return;

    //remove bonus honor aura trigger creature when node is lost
    if (node < BG_AB_DYNAMIC_NODES_COUNT)//only dynamic nodes, no start points
        DeleteCreature(node+7);//NULL checks are in DeleteCreature! 0-6 spirit guides

    // Those who are waiting to resurrect at this node are taken to the closest own node's graveyard
    std::vector<uint64> ghost_list = m_ReviveQueue[m_BgCreatures[node]];
    if (!ghost_list.empty())
    {
        WorldSafeLocsEntry const *ClosestGrave = NULL;
        for (std::vector<uint64>::const_iterator itr = ghost_list.begin(); itr != ghost_list.end(); ++itr)
        {
            Player* plr = sObjectMgr->GetPlayer(*itr);
            if (!plr)
                continue;

            if (!ClosestGrave)                              // cache
                ClosestGrave = GetClosestGraveYard(plr);

            if (ClosestGrave)
                plr->TeleportTo(GetMapId(), ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, plr->GetOrientation());
        }
    }

    if (m_BgCreatures[node])
        DeleteCreature(node);

    // buff object isn't despawned
}

/* Invoked if a player used a banner as a gameobject */
void BattlegroundAB::EventPlayerClickedOnFlag(Player *source, GameObject* /*target_obj*/)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 node = BG_AB_NODE_STABLES;
    GameObject* obj=GetBgMap()->GetGameObject(m_BgObjects[node*8+7]);
    while ((node < BG_AB_DYNAMIC_NODES_COUNT) && ((!obj) || (!source->IsWithinDistInMap(obj, 10))))
    {
        ++node;
        obj=GetBgMap()->GetGameObject(m_BgObjects[node*8+BG_AB_OBJECT_AURA_CONTESTED]);
    }

    if (node == BG_AB_DYNAMIC_NODES_COUNT)
    {
        // this means our player isn't close to any of banners - maybe cheater ??
        return;
    }

    BattlegroundTeamId teamIndex = GetTeamIndexByTeamId(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(_nodes[node] == 0 || teamIndex == _nodes[node]%2))
        return;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    uint32 sound = 0;
    // If node is neutral, change to contested
    if (_nodes[node] == BG_AB_NODE_TYPE_NEUTRAL)
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        _prevNodes[node] = _nodes[node];
        _nodes[node] = teamIndex + 1;
        // burn current neutral banner
        _DelBanner(node, BG_AB_NODE_TYPE_NEUTRAL, 0);
        // create new contested banner
        _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        _nodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

        // FIXME: team and node names not localized
        if (teamIndex == 0)
            SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_CLAIMED, _GetNodeNameId(node), LANG_BG_AB_ALLY), CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
        else
            SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_CLAIMED, _GetNodeNameId(node), LANG_BG_AB_HORDE), CHAT_MSG_BG_SYSTEM_HORDE, source);

        sound = BG_AB_SOUND_NODE_CLAIMED;
    }
    // If node is contested
    else if ((_nodes[node] == BG_AB_NODE_STATUS_ALLY_CONTESTED) || (_nodes[node] == BG_AB_NODE_STATUS_HORDE_CONTESTED))
    {
        // If last state is NOT occupied, change node to enemy-contested
        if (_prevNodes[node] < BG_AB_NODE_TYPE_OCCUPIED)
        {
            UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
            _prevNodes[node] = _nodes[node];
            _nodes[node] = teamIndex + BG_AB_NODE_TYPE_CONTESTED;
            // burn current contested banner
            _DelBanner(node, BG_AB_NODE_TYPE_CONTESTED, !teamIndex);
            // create new contested banner
            _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
            _SendNodeUpdate(node);
            _nodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

            // FIXME: node names not localized
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_ASSAULTED, _GetNodeNameId(node)), CHAT_MSG_BG_SYSTEM_ALLIANCE, source);
            else
                SendMessageToAll(ParseStrings(LANG_BG_AB_NODE_ASSAULTED, _GetNodeNameId(node)), CHAT_MSG_BG_SYSTEM_HORDE, source);
        }
        // If contested, change back to occupied
        else
        {
            UpdatePlayerScore(source, SCORE_BASES_DEFENDED, 1);
            _prevNodes[node] = _nodes[node];
            _nodes[node] = teamIndex + BG_AB_NODE_TYPE_OCCUPIED;
            // burn current contested banner
            _DelBanner(node, BG_AB_NODE_TYPE_CONTESTED, !teamIndex);
            // create new occupied banner
            _CreateBanner(node, BG_AB_NODE_TYPE_OCCUPIED, teamIndex, true);
            _SendNodeUpdate(node);
            _nodeTimers[node] = 0;
            _NodeOccupied(node, (teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE:HORDE);

            // FIXME: node names not localized
            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AB_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AB_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied, change to enemy-contested
    else
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        _prevNodes[node] = _nodes[node];
        _nodes[node] = teamIndex + BG_AB_NODE_TYPE_CONTESTED;
        // burn current occupied banner
        _DelBanner(node, BG_AB_NODE_TYPE_OCCUPIED, !teamIndex);
        // create new contested banner
        _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        _NodeDeOccupied(node);
        _nodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

        // FIXME: node names not localized
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED, CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));

        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AB_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AB_SOUND_NODE_ASSAULTED_HORDE;
    }

    // If node is occupied again, send "X has taken the Y" msg.
    if (_nodes[node] >= BG_AB_NODE_TYPE_OCCUPIED)
    {
        // FIXME: team and node names not localized
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_AB_ALLY, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN, CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_AB_HORDE, _GetNodeNameId(node));
    }
    PlaySoundToAll(sound);
}

void BattlegroundAB::StartBattleground()
{
    BattlegroundMap::StartBattleground();

    // spawn neutral banners
    for (int banner = BG_AB_OBJECT_BANNER_NEUTRAL, i = 0; i < 5; banner += 8, ++i)
        SpawnGameObject(banner, RESPAWN_IMMEDIATELY);

    for (int i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
    {
        //randomly select buff to spawn
        uint8 buff = urand(0, 2);
        SpawnGameObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + buff + i * 3, RESPAWN_IMMEDIATELY);
    }

    DoorOpen(BG_AB_OBJECT_GATE_A);
    DoorOpen(BG_AB_OBJECT_GATE_H);
}

void BattlegroundAB::EndBattleground(BattlegroundWinner winner)
{
    //win reward
    RewardHonorToTeam(GetBonusHonorFromKill(1), winner);

    //complete map reward
    RewardHonorToTeam(GetBonusHonorFromKill(1), BG_TEAM_ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), BG_TEAM_HORDE);

    BattlegroundMap::EndBattleground(winner);
}

WorldSafeLocsEntry const* BattlegroundAB::GetClosestGraveYard(Player* player)
{
    BattlegroundTeamId teamIndex = BattlegroundTeamId(player->GetBGTeam());

    // Is there any occupied node for this team?
    std::vector<uint8> nodes;
    for (uint8 i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
        if (_nodes[i] == teamIndex + 3)
            nodes.push_back(i);

    WorldSafeLocsEntry const* good_entry = NULL;
    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float plr_x = player->GetPositionX();
        float plr_y = player->GetPositionY();

        float mindist = 999999.0f;
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const*entry = sWorldSafeLocsStore.LookupEntry(BG_AB_GraveyardIds[nodes[i]]);
            if (!entry)
                continue;
            float dist = (entry->x - plr_x)*(entry->x - plr_x)+(entry->y - plr_y)*(entry->y - plr_y);
            if (mindist > dist)
            {
                mindist = dist;
                good_entry = entry;
            }
        }
        nodes.clear();
    }
    // If not, place ghost on starting location
    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry(BG_AB_GraveyardIds[teamIndex+5]);

    return good_entry;
}

void BattlegroundAB::UpdatePlayerScore(Player *source, uint32 type, uint32 value, bool addHonor)
{
    BattlegroundScoreMap::iterator itr = PlayerScores.find(source->GetGUIDLow());
    if (itr == PlayerScores.end())                         // player not found...
        return;

    switch (type)
    {
        case SCORE_BASES_ASSAULTED:
            ((BattlegroundABScore*)itr->second)->BasesAssaulted += value;
            source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, AB_OBJECTIVE_ASSAULT_BASE);
            break;
        case SCORE_BASES_DEFENDED:
            ((BattlegroundABScore*)itr->second)->BasesDefended += value;
            source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, AB_OBJECTIVE_DEFEND_BASE);
            break;
        default:
            BattlegroundMap::UpdatePlayerScore(source, type, value, addHonor);
            break;
    }
}

bool BattlegroundAB::IsAllNodesConrolledByTeam(uint32 team) const
{
    uint32 count = 0;
    for (int i = 0; i < BG_AB_DYNAMIC_NODES_COUNT; ++i)
        if ((team == ALLIANCE && _nodes[i] == BG_AB_NODE_STATUS_ALLY_OCCUPIED) ||
            (team == HORDE    && _nodes[i] == BG_AB_NODE_STATUS_HORDE_OCCUPIED))
            ++count;

    return count == BG_AB_DYNAMIC_NODES_COUNT;
}
