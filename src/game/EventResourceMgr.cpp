#include "EventResourceMgr.h"
#include "Policies/SingletonImp.h"
#include "ProgressBar.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "MapManager.h"
#include "CellImpl.h"

INSTANTIATE_SINGLETON_1(EventResourceMgr);

void EventResourceMgr::LoadResourceEvents()
{
    QueryResult* result = WorldDatabase.Query("SELECT event_id, resource_id, resource_full_count FROM event_resource");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outBasic(">> Loaded 0 resource events.");
        return;
    }

    {
        BarGoLink bar(result->GetRowCount());
        uint32 count = 0;

        Field* fields;
        do
        {
            bar.step();

            fields = result->Fetch();
            uint32 event_id = fields[0].GetUInt32();
            uint32 resource_id = fields[1].GetUInt32();
            uint32 resource_full_count = fields[2].GetUInt32();

            m_resourceEvents[event_id][resource_id].full_count = resource_full_count;

            // Initialisation values
            m_resourceEvents[event_id][resource_id].current_count = 0;
            
            ++count;
        } while (result->NextRow());
        delete result;

        sLog.outBasic(">> Loaded %u resource events.", count);
    }

    result = CharacterDatabase.Query("SELECT event_id, resource_id, resource_count FROM event_resource_count");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outBasic(">> Loaded 0 resource counts for the resource events.");
    }
    else
    {
        BarGoLink bar(result->GetRowCount());
        uint32 count = 0;

        Field* fields;
        do
        {
            ++count;

            bar.step();

            fields = result->Fetch();
            uint32 event_id = fields[0].GetUInt32();
            uint32 resource_id = fields[1].GetUInt32();
            uint32 resource_count = fields[2].GetUInt32();

            m_resourceEvents[event_id][resource_id].current_count = resource_count;
        } while (result->NextRow());
        delete result;

        sLog.outBasic(">> Loaded %u resource counts for the resource events.", count);
    }

    result = WorldDatabase.Query("SELECT id, event_id, resource_id, resource_limit, object_guid FROM event_resource_gameobject");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outBasic(">> Loaded 0 resource gameobjects for the resource events.");
    }
    else
    {
        BarGoLink bar(result->GetRowCount());
        uint32 count = 0;

        Field* fields;
        do
        {
            ++count;
            bar.step();

            fields = result->Fetch();
            uint32 event_id = fields[1].GetUInt32();
            uint32 resource_id = fields[2].GetUInt32();
            uint32 resource_limit = fields[3].GetUInt32();
            uint32 object_guid = fields[4].GetUInt32();

            ResourceGameObjectList& list = m_resourceEvents[event_id][resource_id].objects;

            ResourceGameObjectInfo info = { resource_limit, object_guid };
            list.push_back(info);
        } while (result->NextRow());
        delete result;

        sLog.outBasic(">> Loaded %u resource gameobjects for the resource events.", count);
    }

    result = CharacterDatabase.Query("SELECT event_id, completed FROM event_resource_status");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outBasic(">> Loaded 0 resource event statuses for the resource events.");
    }
    else
    {
        BarGoLink bar(result->GetRowCount());
        uint32 count = 0;

        Field* fields;
        do
        {
            ++count;
            bar.step();

            fields = result->Fetch();
            uint32 event_id = fields[0].GetUInt32();
            bool completed = fields[1].GetBool();

            m_resourceEventStatuses[event_id]= completed;

        } while (result->NextRow());
        delete result;

        sLog.outBasic(">> Loaded %u resource event statuses for the resource events.", count);
    }

    // Load the initial states for all the objects.
    for (std::pair<const uint32, ResourceEvent>& eventPair : m_resourceEvents)
        CheckSpawnGOEvent(eventPair.first);

    sLog.outBasic(">> Finished loading the resource events.");
}

bool EventResourceMgr::AddResourceCount(uint32 event_id, uint32 resource_id, int count)
{
    bool notDone = true;
    ResourceType& resource_type = m_resourceEvents.at(event_id).at(resource_id);

    if (resource_type.current_count + count >= resource_type.full_count)
        notDone = false;

    resource_type.current_count += count;

    SaveResourceCount(event_id, resource_id, resource_type.current_count);

    return notDone;
}

uint32 EventResourceMgr::GetResourceCount(uint32 event_id, uint32 resource_id)
{
    ResourceType& resource_type = m_resourceEvents.at(event_id).at(resource_id);

    return resource_type.current_count;
}

uint32 EventResourceMgr::GetFullResourceCount(uint32 event_id, uint32 resource_id)
{
    ResourceType& resource_type = m_resourceEvents.at(event_id).at(resource_id);

    return resource_type.full_count;
}

void EventResourceMgr::ChangeAllResourcesByPercentage(uint32 event_id, float percentage)
{
    for (std::pair<const uint32, ResourceType>& resourcePair : m_resourceEvents[event_id])
    {
        ResourceType& resource = resourcePair.second;

        if (percentage * resource.full_count <= resource.current_count)
            resource.current_count  -= percentage * resource.full_count;

        SaveResourceCount(event_id, resourcePair.first, resource.current_count);
    }
}

void EventResourceMgr::CheckSpawnGOEvent(uint32 event_id)
{
    ResourceEvent& resourceEvent = m_resourceEvents[event_id];

    // Sum the total needed and the total gathered of each resource.
    // second contains the needed limit and first contains the number
    // of currently gatherered resources.
    std::map<uint32, std::pair<uint32, uint32> > resourceSum;

    for (std::pair<const uint32, ResourceType>& resourcePair : resourceEvent)
    {
        ResourceType& resource = resourcePair.second;

        // Sum all the requirements for that gameObject.
        for (ResourceGameObjectInfo& goInfo : resource.objects)
        {
            resourceSum[goInfo.object_guid].first += resource.current_count;
            resourceSum[goInfo.object_guid].second += goInfo.trigger_limit;
        }
    }

    for (std::pair<const uint32, std::pair<uint32, uint32> >& resourceSumPair : resourceSum)
    {
        uint32 guid = resourceSumPair.first;
        uint32 current_count = resourceSumPair.second.first;
        uint32 trigger_limit = resourceSumPair.second.second;

        const GameObjectData* pData = sObjectMgr.GetGOData(guid);
        if (pData)
        {
            Map* pMap = sMapMgr.FindMap(pData->mapid);
            if (pMap)
            {
                if (!pMap->IsLoaded(pData->posX, pData->posY))
                {
                    MaNGOS::ObjectUpdater updater(0);
                    // for creature
                    TypeContainerVisitor<MaNGOS::ObjectUpdater, GridTypeMapContainer  > grid_object_update(updater);
                    // for pets
                    TypeContainerVisitor<MaNGOS::ObjectUpdater, WorldTypeMapContainer > world_object_update(updater);
                    // Make sure that the creature is loaded before checking its status.
                    CellPair cellPair = MaNGOS::ComputeCellPair(pData->posX, pData->posY);
                    Cell cell(cellPair);
                    pMap->Visit(cell, grid_object_update);
                    pMap->Visit(cell, world_object_update);
                }

                GameObject* pObject = pMap->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, pData->id, guid));

                // If the object isn't spawned and the limit is reached; spawn it.
                // Otherwise the gameobject should be despawned if it's under the limit.
                if (!pObject && current_count >= trigger_limit)
                {
                    sLog.outBasic("EventResourceMgr: The limit of %u has been reached for %u. Spawning it!", 
                            trigger_limit, guid);

                    sObjectMgr.AddGameobjectToGrid(guid, pData);
                    GameObject::SpawnInMaps(guid, pData);

                }
            }

            if (current_count < trigger_limit)
            {
                sObjectMgr.RemoveGameobjectFromGrid(guid, pData);
                GameObject::AddToRemoveListInMaps(guid, pData);
            }

        }
        else
            sLog.outError("EventResourceMgr: Could not find the GameObject with GUID %u!", 
                          guid);

    }
}

bool EventResourceMgr::IsEventCompleted(uint32 event_id)
{
    if (m_resourceEventStatuses[event_id])
        return true;

    bool complete = true;
    for (std::pair<const uint32, ResourceType>& resource_pair : m_resourceEvents[event_id])
    {
        ResourceType& resource = resource_pair.second;

        if (resource.current_count < resource.full_count)
        {
            complete = false;
            break;
        }
    }

    if (complete)
    {
        CharacterDatabase.PQuery("REPLACE INTO event_resource_status (`event_id`, `completed`) VALUES (%u, b'1')", event_id);
        m_resourceEventStatuses[event_id] = true;
    }

    return complete;
}

void EventResourceMgr::SaveResourceCount(uint32 event_id, uint32 resource_id, uint32 value)
{
    CharacterDatabase.PQuery("REPLACE INTO event_resource_count (`event_id`, `resource_id`,"
                             " `resource_count`) VALUES ('%u', '%u', '%u')", 
                             event_id, resource_id, value);

    CheckSpawnGOEvent(event_id);
}

bool AddResourceCount(uint32 event_id, uint32 resource_id, int count)
{
    return sEventResourceMgr.AddResourceCount(event_id, resource_id, count);
}

uint32 GetResourceCount(uint32 event_id, uint32 resource_id)
{
    return sEventResourceMgr.GetResourceCount(event_id, resource_id);
}

uint32 GetFullResourceCount(uint32 event_id, uint32 resource_id)
{
    return sEventResourceMgr.GetFullResourceCount(event_id, resource_id);
}

void ChangeAllResourcesByPercentage(uint32 event_id, float percentage)
{
    sEventResourceMgr.ChangeAllResourcesByPercentage(event_id, percentage);
}

bool IsEventCompleted(uint32 event_id)
{
    return sEventResourceMgr.IsEventCompleted(event_id);
}
