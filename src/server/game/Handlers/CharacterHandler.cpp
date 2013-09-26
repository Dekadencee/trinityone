/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "AccountMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Language.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "PlayerDump.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "SystemConfig.h"
#include "UpdateMask.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"


class LoginQueryHolder : public SQLQueryHolder
{
    private:
        uint32 m_accountId;
        uint64 m_guid;
    public:
        LoginQueryHolder(uint32 accountId, uint64 guid)
            : m_accountId(accountId), m_guid(guid) { }
        uint64 GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
};

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    uint32 lowGuid = GUID_LOPART(m_guid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BOUND_INSTANCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DAILYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_WEEKLYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MONTHLYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SEASONALQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILCOUNT);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt64(1, uint64(time(NULL)));
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_COUNT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MAILDATE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_DATE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->setUInt32(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BGDATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BG_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

    return res;
}

void WorldSession::HandleCharEnum(PreparedQueryResult result)
{
    WorldPacket data(SMSG_CHAR_ENUM, 100);                  // we guess size

    uint8 num = 0;

    data << num;

    _legitCharacters.clear();
    if (result)
    {
        do
        {
            uint32 guidlow = (*result)[0].GetUInt32();
            TC_LOG_INFO(LOG_FILTER_NETWORKIO, "Loading char guid %u from account %u.", guidlow, GetAccountId());
            if (Player::BuildEnumData(result, &data))
            {
                // Do not allow banned characters to log in
                if (!(*result)[20].GetUInt32())
                    _legitCharacters.insert(guidlow);

                if (!sWorld->HasCharacterNameData(guidlow)) // This can happen if characters are inserted into the database manually. Core hasn't loaded name data yet.
                    sWorld->AddCharacterNameData(guidlow, (*result)[1].GetString(), (*result)[4].GetUInt8(), (*result)[2].GetUInt8(), (*result)[3].GetUInt8(), (*result)[7].GetUInt8());
                ++num;
            }
        }
        while (result->NextRow());
    }

    data.put<uint8>(0, num);

    SendPacket(&data);
}

void WorldSession::HandleCharEnumOpcode(WorldPacket & /*recvData*/)
{
    AntiDOS.AllowOpcode(CMSG_CHAR_ENUM, false);

    // remove expired bans
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
    CharacterDatabase.Execute(stmt);

    /// get all the data necessary for loading all characters (along with their pets) on the account

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

    stmt->setUInt8(0, PET_SAVE_AS_CURRENT);
    stmt->setUInt32(1, GetAccountId());

    _charEnumCallback = CharacterDatabase.AsyncQuery(stmt);
}

void WorldSession::HandleCharCreateOpcode(WorldPacket& recvData)
{
    std::string name;
    uint8 race_, class_;

    recvData >> name;

    recvData >> race_;
    recvData >> class_;

    // extract other data required for player creating
    uint8 gender, skin, face, hairStyle, hairColor, facialHair, outfitId;
    recvData >> gender >> skin >> face;
    recvData >> hairStyle >> hairColor >> facialHair >> outfitId;

    WorldPacket data(SMSG_CHAR_CREATE, 1);                  // returned with diff.values in all cases

    if (!HasPermission(RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_TEAMMASK))
    {
        if (uint32 mask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED))
        {
            bool disabled = false;

            uint32 team = Player::TeamForRace(race_);
            switch (team)
            {
                case ALLIANCE:
                    disabled = mask & (1 << 0);
                    break;
                case HORDE:
                    disabled = mask & (1 << 1);
                    break;
            }

            if (disabled)
            {
                data << uint8(CHAR_CREATE_DISABLED);
                SendPacket(&data);
                return;
            }
        }
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(class_);
    if (!classEntry)
    {
        data << uint8(CHAR_CREATE_FAILED);
        SendPacket(&data);
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Class (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", class_, GetAccountId());
        return;
    }

    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race_);
    if (!raceEntry)
    {
        data << uint8(CHAR_CREATE_FAILED);
        SendPacket(&data);
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Race (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", race_, GetAccountId());
        return;
    }

    // prevent character creating Expansion race without Expansion account
    if (raceEntry->expansion > Expansion())
    {
        data << uint8(CHAR_CREATE_EXPANSION);
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Expansion %u account:[%d] tried to Create character with expansion %u race (%u)", Expansion(), GetAccountId(), raceEntry->expansion, race_);
        SendPacket(&data);
        return;
    }

    // prevent character creating Expansion class without Expansion account
    if (classEntry->expansion > Expansion())
    {
        data << uint8(CHAR_CREATE_EXPANSION_CLASS);
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Expansion %u account:[%d] tried to Create character with expansion %u class (%u)", Expansion(), GetAccountId(), classEntry->expansion, class_);
        SendPacket(&data);
        return;
    }

    if (!HasPermission(RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (race_ - 1)) & raceMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(&data);
            return;
        }
    }

    if (!HasPermission(RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_CLASSMASK))
    {
        uint32 classMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
        if ((1 << (class_ - 1)) & classMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(&data);
            return;
        }
    }

    // prevent character creating with invalid name
    if (!normalizePlayerName(name))
    {
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Account:[%d] but tried to Create character with empty [name] ", GetAccountId());
        return;
    }

    // check name limitations
    uint8 res = ObjectMgr::CheckPlayerName(name, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    if (!HasPermission(RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(name))
    {
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    delete _charCreateCallback.GetParam();  // Delete existing if any, to make the callback chain reset to stage 0
    _charCreateCallback.SetParam(new CharacterCreateInfo(name, race_, class_, gender, skin, face, hairStyle, hairColor, facialHair, outfitId, recvData));
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
    stmt->setString(0, name);
    _charCreateCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
}

void WorldSession::HandleCharCreateCallback(PreparedQueryResult result, CharacterCreateInfo* createInfo)
{
    /** This is a series of callbacks executed consecutively as a result from the database becomes available.
        This is much more efficient than synchronous requests on packet handler, and much less DoS prone.
        It also prevents data syncrhonisation errors.
    */
    switch (_charCreateCallback.GetStage())
    {
        case 0:
        {
            if (result)
            {
                WorldPacket data(SMSG_CHAR_CREATE, 1);
                data << uint8(CHAR_CREATE_NAME_IN_USE);
                SendPacket(&data);
                delete createInfo;
                _charCreateCallback.Reset();
                return;
            }

            ASSERT(_charCreateCallback.GetParam() == createInfo);

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_SUM_REALM_CHARACTERS);
            stmt->setUInt32(0, GetAccountId());

            _charCreateCallback.FreeResult();
            _charCreateCallback.SetFutureResult(LoginDatabase.AsyncQuery(stmt));
            _charCreateCallback.NextStage();
        }
        break;
        case 1:
        {
            uint16 acctCharCount = 0;
            if (result)
            {
                Field* fields = result->Fetch();
                // SELECT SUM(x) is MYSQL_TYPE_NEWDECIMAL - needs to be read as string
                const char* ch = fields[0].GetCString();
                if (ch)
                    acctCharCount = atoi(ch);
            }

            if (acctCharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT))
            {
                WorldPacket data(SMSG_CHAR_CREATE, 1);
                data << uint8(CHAR_CREATE_ACCOUNT_LIMIT);
                SendPacket(&data);
                delete createInfo;
                _charCreateCallback.Reset();
                return;
            }


            ASSERT(_charCreateCallback.GetParam() == createInfo);

            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
            stmt->setUInt32(0, GetAccountId());

            _charCreateCallback.FreeResult();
            _charCreateCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
            _charCreateCallback.NextStage();
        }
        break;
        case 2:
        {
            if (result)
            {
                Field* fields = result->Fetch();
                createInfo->CharCount = uint8(fields[0].GetUInt64()); // SQL's COUNT() returns uint64 but it will always be less than uint8.Max

                if (createInfo->CharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
                {
                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                    data << uint8(CHAR_CREATE_SERVER_LIMIT);
                    SendPacket(&data);
                    delete createInfo;
                    _charCreateCallback.Reset();
                    return;
                }
            }

            bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
            uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

            _charCreateCallback.FreeResult();

            if (!allowTwoSideAccounts || skipCinematics == 1)
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CREATE_INFO);
                stmt->setUInt32(0, GetAccountId());
                stmt->setUInt32(1, (skipCinematics == 1));
                _charCreateCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
                _charCreateCallback.NextStage();
                return;
            }

            _charCreateCallback.NextStage();
            HandleCharCreateCallback(PreparedQueryResult(NULL), createInfo);   // Will jump to case 3
        }
        break;
        case 3:
        {
            bool haveSameRace = false;
            uint32 heroicReqLevel = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
            bool hasHeroicReqLevel = (heroicReqLevel == 0);
            bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
            uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

            if (result)
            {
                uint32 team = Player::TeamForRace(createInfo->Race);

                Field* field = result->Fetch();
                uint8 accRace  = field[1].GetUInt8();

                // need to check team only for first character
                /// @todo what to if account already has characters of both races?
                if (!allowTwoSideAccounts)
                {
                    uint32 accTeam = 0;
                    if (accRace > 0)
                        accTeam = Player::TeamForRace(accRace);

                    if (accTeam != team)
                    {
                        WorldPacket data(SMSG_CHAR_CREATE, 1);
                        data << uint8(CHAR_CREATE_PVP_TEAMS_VIOLATION);
                        SendPacket(&data);
                        delete createInfo;
                        _charCreateCallback.Reset();
                        return;
                    }
                }

                // search same race for cinematic or same class if need
                /// @todo check if cinematic already shown? (already logged in?; cinematic field)
                while (skipCinematics == 1 && !haveSameRace)
                {
                    if (!result->NextRow())
                        break;

                    field = result->Fetch();
                    accRace = field[1].GetUInt8();

                    if (!haveSameRace)
                        haveSameRace = createInfo->Race == accRace;

                }
            }

            if (createInfo->Data.rpos() < createInfo->Data.wpos())
            {
                uint8 unk;
                createInfo->Data >> unk;
                TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "Character creation %s (account %u) has unhandled tail data: [%u]", createInfo->Name.c_str(), GetAccountId(), unk);
            }

            Player newChar(this);
            newChar.GetMotionMaster()->Initialize();
            if (!newChar.Create(sObjectMgr->GenerateLowGuid(HIGHGUID_PLAYER), createInfo))
            {
                // Player not create (race/class/etc problem?)
                newChar.CleanupsBeforeDelete();

                WorldPacket data(SMSG_CHAR_CREATE, 1);
                data << uint8(CHAR_CREATE_ERROR);
                SendPacket(&data);
                delete createInfo;
                _charCreateCallback.Reset();
                return;
            }

            if ((haveSameRace && skipCinematics == 1) || skipCinematics == 2)
                newChar.setCinematic(1);                          // not show intro

            newChar.SetAtLoginFlag(AT_LOGIN_FIRST);               // First login

            // Player created, save it now
            newChar.SaveToDB(true);
            createInfo->CharCount += 1;

            SQLTransaction trans = LoginDatabase.BeginTransaction();

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM);
            stmt->setUInt32(0, GetAccountId());
            stmt->setUInt32(1, realmID);
            trans->Append(stmt);

            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
            stmt->setUInt32(0, createInfo->CharCount);
            stmt->setUInt32(1, GetAccountId());
            stmt->setUInt32(2, realmID);
            trans->Append(stmt);

            LoginDatabase.CommitTransaction(trans);

            WorldPacket data(SMSG_CHAR_CREATE, 1);
            data << uint8(CHAR_CREATE_SUCCESS);
            SendPacket(&data);

            AntiDOS.AllowOpcode(CMSG_CHAR_ENUM, true);
            std::string IP_str = GetRemoteAddress();
            TC_LOG_INFO(LOG_FILTER_CHARACTER, "Account: %d (IP: %s) Create Character:[%s] (GUID: %u)", GetAccountId(), IP_str.c_str(), createInfo->Name.c_str(), newChar.GetGUIDLow());
            sScriptMgr->OnPlayerCreate(&newChar);
            sWorld->AddCharacterNameData(newChar.GetGUIDLow(), newChar.GetName(), newChar.getGender(), newChar.getRace(), newChar.getClass(), newChar.getLevel());

            newChar.CleanupsBeforeDelete();
            delete createInfo;
            _charCreateCallback.Reset();
        }
        break;
    }
}

void WorldSession::HandleCharDeleteOpcode(WorldPacket& recvData)
{
    uint64 guid;
    recvData >> guid;

    // can't delete loaded character
    if (ObjectAccessor::FindPlayer(guid))
        return;

    uint32 accountId = 0;
    uint8 level = 0;
    std::string name;

    // is guild leader
    if (sGuildMgr->GetGuildByLeader(guid))
    {
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << uint8(CHAR_DELETE_FAILED_GUILD_LEADER);
        SendPacket(&data);
        return;
    }

    // is arena team captain
    if (sArenaTeamMgr->GetArenaTeamByCaptain(guid))
    {
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << uint8(CHAR_DELETE_FAILED_ARENA_CAPTAIN);
        SendPacket(&data);
        return;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DATA_BY_GUID);
    stmt->setUInt32(0, GUID_LOPART(guid));

    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        Field* fields = result->Fetch();
        accountId = fields[0].GetUInt32();
        name = fields[1].GetString();
        level = fields[2].GetUInt8();
    }

    // prevent deleting other players' characters using cheating tools
    if (accountId != GetAccountId())
        return;

    std::string IP_str = GetRemoteAddress();
    TC_LOG_INFO(LOG_FILTER_CHARACTER, "Account: %d, IP: %s deleted character: %s, GUID: %u, Level: %u", accountId, IP_str.c_str(), name.c_str(), GUID_LOPART(guid), level);
    sScriptMgr->OnPlayerDelete(guid);

    if (sLog->ShouldLog(LOG_FILTER_PLAYER_DUMP, LOG_LEVEL_INFO)) // optimize GetPlayerDump call
    {
        std::string dump;
        if (PlayerDumpWriter().GetDump(GUID_LOPART(guid), dump))
            sLog->outCharDump(dump.c_str(), accountId, GUID_LOPART(guid), name.c_str());
    }

    Player::DeleteFromDB(guid, accountId);

    WorldPacket data(SMSG_CHAR_DELETE, 1);
    data << uint8(CHAR_DELETE_SUCCESS);
    SendPacket(&data);

    AntiDOS.AllowOpcode(CMSG_CHAR_ENUM, true);
}

void WorldSession::HandlePlayerLoginOpcode(WorldPacket& recvData)
{
    if (PlayerLoading() || GetPlayer() != NULL)
    {
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Player tryes to login again, AccountId = %d", GetAccountId());
        return;
    }

    m_playerLoading = true;
    uint64 playerGuid = 0;

    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Recvd Player Logon Message");

    recvData >> playerGuid;

    if (!IsLegitCharacterForAccount(GUID_LOPART(playerGuid)))
    {
        TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Account (%u) can't login with that character (%u).", GetAccountId(), GUID_LOPART(playerGuid));
        KickPlayer();
        return;
    }

    LoginQueryHolder *holder = new LoginQueryHolder(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    _charLoginCallback = CharacterDatabase.DelayQueryHolder((SQLQueryHolder*)holder);
}

void WorldSession::HandlePlayerLogin(LoginQueryHolder* holder)
{
    uint64 playerGuid = holder->GetGuid();

    Player* pCurrChar = new Player(this);
     // for send server info and strings (config)
    ChatHandler chH = ChatHandler(pCurrChar->GetSession());

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    if (!pCurrChar->LoadFromDB(GUID_LOPART(playerGuid), holder))
    {
        SetPlayer(NULL);
        KickPlayer();                                       // disconnect client, player no set to session and it will not deleted or saved at kick
        delete pCurrChar;                                   // delete it manually
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    pCurrChar->GetMotionMaster()->Initialize();
    pCurrChar->SendDungeonDifficulty(false);

    WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20);
    data << pCurrChar->GetMapId();
    data << pCurrChar->GetPositionX();
    data << pCurrChar->GetPositionY();
    data << pCurrChar->GetPositionZ();
    data << pCurrChar->GetOrientation();
    SendPacket(&data);

    // load player specific part before send times
    LoadAccountData(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    data.Initialize(SMSG_FEATURE_SYSTEM_STATUS, 2);         // added in 2.2.0
    data << uint8(2);                                       // unknown value
    data << uint8(0);                                       // enable(1)/disable(0) voice chat interface in client
    SendPacket(&data);

    // Send MOTD
    {
        data.Initialize(SMSG_MOTD, 50);                     // new in 2.0.1
        data << (uint32)0;

        uint32 linecount=0;
        std::string str_motd = sWorld->GetMotd();
        std::string::size_type pos, nextpos;

        pos = 0;
        while ((nextpos= str_motd.find('@', pos)) != std::string::npos)
        {
            if (nextpos != pos)
            {
                data << str_motd.substr(pos, nextpos-pos);
                ++linecount;
            }
            pos = nextpos+1;
        }

        if (pos<str_motd.length())
        {
            data << str_motd.substr(pos);
            ++linecount;
        }

        data.put(0, linecount);

        SendPacket(&data);
        TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Sent motd (SMSG_MOTD)");

        // send server info
        if (sWorld->getIntConfig(CONFIG_ENABLE_SINFO_LOGIN) == 1)
            chH.PSendSysMessage(_FULLVERSION);

        TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Sent server info");
    }

    //QueryResult* result = CharacterDatabase.PQuery("SELECT guildid, rank FROM guild_member WHERE guid = '%u'", pCurrChar->GetGUIDLow());
    if (PreparedQueryResult resultGuild = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_GUILD))
    {
        Field* fields = resultGuild->Fetch();
        pCurrChar->SetInGuild(fields[0].GetUInt32());
        pCurrChar->SetRank(fields[1].GetUInt8());
    }
    else if (pCurrChar->GetGuildId())                        // clear guild related fields in case wrong data about non existed membership
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
    }

    if (pCurrChar->GetGuildId() != 0)
    {
        if (Guild* guild = sGuildMgr->GetGuildById(pCurrChar->GetGuildId()))
            guild->SendLoginInfo(this);
        else
        {
            // remove wrong guild data
            TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "Player %s (GUID: %u) marked as member of not existing guild (id: %u), removing guild membership for player.", pCurrChar->GetName().c_str(), pCurrChar->GetGUIDLow(), pCurrChar->GetGuildId());
            pCurrChar->SetInGuild(0);
        }
    }

    pCurrChar->SendInitialPacketsBeforeAddToMap();

    //Show cinematic at the first time that player login
    if (!pCurrChar->getCinematic())
    {
        pCurrChar->setCinematic(1);

        if (ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(pCurrChar->getClass()))
        {
            if (cEntry->CinematicSequence)
                pCurrChar->SendCinematicStart(cEntry->CinematicSequence);
            else if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(pCurrChar->getRace()))
                pCurrChar->SendCinematicStart(rEntry->CinematicSequence);

            // send new char string if not empty
            if (!sWorld->GetNewCharString().empty())
                chH.PSendSysMessage("%s", sWorld->GetNewCharString().c_str());
        }
    }

    if (!pCurrChar->GetMap()->AddPlayerToMap(pCurrChar) || !pCurrChar->CheckInstanceLoginValid())
    {
        AreaTrigger const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
        if (at)
            pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
        else
            pCurrChar->TeleportTo(pCurrChar->m_homebindMapId, pCurrChar->m_homebindX, pCurrChar->m_homebindY, pCurrChar->m_homebindZ, pCurrChar->GetOrientation());
    }

    sObjectAccessor->AddObject(pCurrChar);
    //TC_LOG_DEBUG("Player %s added to Map.", pCurrChar->GetName().c_str());

    pCurrChar->SendInitialPacketsAfterAddToMap();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);

    stmt->setUInt32(0, pCurrChar->GetGUIDLow());

    CharacterDatabase.Execute(stmt);

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_ONLINE);

    stmt->setUInt32(0, GetAccountId());

    LoginDatabase.Execute(stmt);

    pCurrChar->SetInGameTime(getMSTime());

    // announce group about member online (must be after add to player list to receive announce to self)
    if (Group* group = pCurrChar->GetGroup())
    {
        //pCurrChar->groupInfo.group->SendInit(this); // useless
        group->SendUpdate();
        group->ResetMaxEnchantingLevel();
    }

    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar, FRIEND_ONLINE, pCurrChar->GetGUIDLow(), true);

    // Place character in world (and load zone) before some object loading
    pCurrChar->LoadCorpse();

    // setting Ghost+speed if dead
    if (pCurrChar->m_deathState != ALIVE)
    {
        // not blizz like, we must correctly save and load player instead...
        if (pCurrChar->getRace() == RACE_NIGHTELF)
            pCurrChar->CastSpell(pCurrChar, 20584, true, 0);// auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
        pCurrChar->CastSpell(pCurrChar, 8326, true, 0);     // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

        pCurrChar->SetMovement(MOVE_WATER_WALK);
    }

    pCurrChar->ContinueTaxiFlight();

    // reset for all pets before pet loading
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        Pet::resetTalentsForAllPetsOf(pCurrChar);

    // Load pet if any (if player not alive and in taxi flight or another then pet will remember as temporary unsummoned)
    pCurrChar->LoadPet();

    // Set FFA PvP for non GM in non-rest mode
    if (sWorld->IsFFAPvPRealm() && !pCurrChar->IsGameMaster() && !pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
        pCurrChar->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

    if (pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
        pCurrChar->SetContestedPvP();

    // Apply at_login requests
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        pCurrChar->resetSpells();
        SendNotification(LANG_RESET_SPELLS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        pCurrChar->resetTalents(true);
        pCurrChar->SendTalentsInfoData(false);              // original talents send already in to SendInitialPacketsBeforeAddToMap, resend reset state
        SendNotification(LANG_RESET_TALENTS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST))
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_FIRST);

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShuttingDown())
        sWorld->ShutdownMsg(true, pCurrChar);

    if (sWorld->getBoolConfig(CONFIG_ALL_TAXI_PATHS))
        pCurrChar->SetTaxiCheater(true);

    if (pCurrChar->IsGameMaster())
        SendNotification(LANG_GM_ON);

    std::string IP_str = GetRemoteAddress();
    TC_LOG_INFO(LOG_FILTER_CHARACTER, "Account: %d (IP: %s) Login Character:[%s] (GUID: %u) Level: %d",
        GetAccountId(), IP_str.c_str(), pCurrChar->GetName().c_str(), pCurrChar->GetGUIDLow(), pCurrChar->getLevel());

    if (!pCurrChar->IsStandState() && !pCurrChar->HasUnitState(UNIT_STATE_STUNNED))
        pCurrChar->SetStandState(UNIT_STAND_STATE_STAND);

    m_playerLoading = false;

    sScriptMgr->OnPlayerLogin(pCurrChar);
    delete holder;
}

void WorldSession::HandleSetFactionAtWar(WorldPacket& recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SET_FACTION_ATWAR");

    uint32 repListID;
    uint8  flag;

    recvData >> repListID;
    recvData >> flag;

    GetPlayer()->GetReputationMgr().SetAtWar(repListID, flag);
}

//I think this function is never used :/ I dunno, but i guess this opcode not exists
void WorldSession::HandleSetFactionCheat(WorldPacket & /*recvData*/)
{
    TC_LOG_ERROR(LOG_FILTER_NETWORKIO, "WORLD SESSION: HandleSetFactionCheat, not expected call, please report.");
    GetPlayer()->GetReputationMgr().SendStates();
}

void WorldSession::HandleTutorialFlag(WorldPacket& recvData)
{
    uint32 data;
    recvData >> data;

    uint8 index = uint8(data / 32);
    if (index >= MAX_ACCOUNT_TUTORIAL_VALUES)
        return;

    uint32 value = (data % 32);

    uint32 flag = GetTutorialInt(index);
    flag |= (1 << value);
    SetTutorialInt(index, flag);
}

void WorldSession::HandleTutorialClear(WorldPacket & /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0xFFFFFFFF);
}

void WorldSession::HandleTutorialReset(WorldPacket & /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0x00000000);
}

void WorldSession::HandleSetWatchedFactionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SET_WATCHED_FACTION");
    uint32 fact;
    recvData >> fact;
    GetPlayer()->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fact);
}

void WorldSession::HandleSetFactionInactiveOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SET_FACTION_INACTIVE");
    uint32 replistid;
    uint8 inactive;
    recvData >> replistid >> inactive;

    _player->GetReputationMgr().SetInactive(replistid, inactive);
}

void WorldSession::HandleShowingHelmOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "CMSG_SHOWING_HELM for %s", _player->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM);
}

void WorldSession::HandleShowingCloakOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "CMSG_SHOWING_CLOAK for %s", _player->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK);
}

void WorldSession::HandleCharRenameOpcode(WorldPacket& recvData)
{
    uint64 guid;
    std::string newName;

    recvData >> guid;
    recvData >> newName;

    // prevent character rename to invalid name
    if (!normalizePlayerName(newName))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newName, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1+8+(newName.size()+1));
        data << uint8(res);
        data << uint64(guid);
        data << newName;
        SendPacket(&data);
        return;
    }

    // check name limitations
    if (!HasPermission(RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(newName))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    // Ensure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name
    _charRenameCallback.SetParam(newName);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_FREE_NAME);

    stmt->setUInt32(0, GUID_LOPART(guid));
    stmt->setUInt32(1, GetAccountId());
    stmt->setUInt16(2, AT_LOGIN_RENAME);
    stmt->setUInt16(3, AT_LOGIN_RENAME);
    stmt->setString(4, newName);

    _charRenameCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
}

void WorldSession::HandleChangePlayerNameOpcodeCallBack(PreparedQueryResult result, std::string const& newName)
{
    AntiDOS.AllowOpcode(CMSG_CHAR_ENUM, true);
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    Field* fields = result->Fetch();

    uint32 guidLow      = fields[0].GetUInt32();
    std::string oldName = fields[1].GetString();

    uint64 guid = MAKE_NEW_GUID(guidLow, 0, HIGHGUID_PLAYER);

    // Update name and at_login flag in the db
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NAME);

    stmt->setString(0, newName);
    stmt->setUInt16(1, AT_LOGIN_RENAME);
    stmt->setUInt32(2, guidLow);

    CharacterDatabase.Execute(stmt);

    // Removed declined name from db
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);

    stmt->setUInt32(0, guidLow);

    CharacterDatabase.Execute(stmt);

    TC_LOG_INFO(LOG_FILTER_CHARACTER, "Account: %d (IP: %s) Character:[%s] (guid:%u) Changed name to: %s", GetAccountId(), GetRemoteAddress().c_str(), oldName.c_str(), guidLow, newName.c_str());

    WorldPacket data(SMSG_CHAR_RENAME, 1+8+(newName.size()+1));
    data << uint8(RESPONSE_SUCCESS);
    data << uint64(guid);
    data << newName;
    SendPacket(&data);

    sWorld->UpdateCharacterNameData(guidLow, newName);
}

void WorldSession::HandleSetPlayerDeclinedNames(WorldPacket& recvData)
{
    uint64 guid;

    recvData >> guid;

    // not accept declined names for unsupported languages
    std::string name;
    if (!sObjectMgr->GetPlayerNameByGUID(guid, name))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                      // name already stored as only single alphabet using
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    std::string name2;
    DeclinedName declinedname;

    recvData >> name2;

    if (name2 != name)                                       // character have different name
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        recvData >> declinedname.name[i];
        if (!normalizePlayerName(declinedname.name[i]))
        {
            WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
            data << uint32(1);
            data << uint64(guid);
            SendPacket(&data);
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(wname, declinedname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        CharacterDatabase.EscapeString(declinedname.name[i]);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, GUID_LOPART(guid));
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, GUID_LOPART(guid));

    for (uint8 i = 0; i < 5; i++)
        stmt->setString(i+1, declinedname.name[i]);

    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
    data << uint32(0);                                      // OK
    data << uint64(guid);
    SendPacket(&data);
}

void WorldSession::HandleEquipmentSetSave(WorldPacket &recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "CMSG_EQUIPMENT_SET_SAVE");

    uint64 setGuid;
    recvData.readPackGUID(setGuid);

    uint32 index;
    recvData >> index;
    if (index >= MAX_EQUIPMENT_SET_INDEX)                    // client set slots amount
        return;

    std::string name;
    recvData >> name;

    std::string iconName;
    recvData >> iconName;

    EquipmentSet eqSet;

    eqSet.Guid      = setGuid;
    eqSet.Name      = name;
    eqSet.IconName  = iconName;
    eqSet.state     = EQUIPMENT_SET_NEW;

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint64 itemGuid;
        recvData.readPackGUID(itemGuid);

        // equipment manager sends "1" (as raw GUID) for slots set to "ignore" (don't touch slot at equip set)
        if (itemGuid == 1)
        {
            // ignored slots saved as bit mask because we have no free special values for Items[i]
            eqSet.IgnoreMask |= 1 << i;
            continue;
        }

        Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (!item && itemGuid)                               // cheating check 1
            return;

        if (item && item->GetGUID() != itemGuid)             // cheating check 2
            return;

        eqSet.Items[i] = GUID_LOPART(itemGuid);
    }

    _player->SetEquipmentSet(index, eqSet);
}

void WorldSession::HandleEquipmentSetDelete(WorldPacket &recvData)
{
    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "CMSG_EQUIPMENT_SET_DELETE");

    uint64 setGuid;
    recvData.readPackGUID(setGuid);

    _player->DeleteEquipmentSet(setGuid);
}

/*
void WorldSession::HandleEquipmentSetUse(WorldPacket &recvData)
{
    if (_player->IsInCombat())
        return;

    TC_LOG_DEBUG(LOG_FILTER_NETWORKIO, "CMSG_EQUIPMENT_SET_USE");

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint64 itemGuid;
        recvData.readPackGUID(itemGuid);

        uint8 srcbag, srcslot;
        recvData >> srcbag >> srcslot;

        TC_LOG_DEBUG(LOG_FILTER_PLAYER_ITEMS, "Item " UI64FMTD ": srcbag %u, srcslot %u", itemGuid, srcbag, srcslot);

        // check if item slot is set to "ignored" (raw value == 1), must not be unequipped then
        if (itemGuid == 1)
            continue;

        Item* item = _player->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        if (!item)
        {
            Item* uItem = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!uItem)
                continue;

            ItemPosCountVec sDest;
            InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                _player->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                _player->StoreItem(sDest, uItem, true);
            }
            else
                _player->SendEquipError(msg, uItem, NULL);

            continue;
        }

        if (item->GetPos() == dstpos)
            continue;

        _player->SwapItem(item->GetPos(), dstpos);
    }

    WorldPacket data(SMSG_EQUIPMENT_SET_USE_RESULT, 1);
    data << uint8(0);                                       // 4 - equipment swap failed - inventory is full
    SendPacket(&data);
}
*/ // Me thinks this has nothing to do with 2.4.3, but I am just going to comment it anyway.
