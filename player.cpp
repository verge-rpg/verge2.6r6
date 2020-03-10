/*
    it's pretty clear that the player could easily be a subclassed entity.  However, that's not
    an option, as the player could be any entity, and it could change at any time.
*/

#include "player.h"
#include "Map/MapFile.h"
#include "engine.h"
#include "entity.h"
#include "verge.h"

using Map::MapFile;

namespace Player
{

    void MovePlayer(Entity& player, Direction direction)
    {
        player.Move(direction);
    }

    void CheckZones(Entity& player)
    {
        int z = map->Zone().Get(player.x/16, player.y/16);

        if (!z) return; // no zone activation to do here.

        MapFile::ZoneInfo& zone = map->GetZoneInfo(z);
        if (zone.adjactivate)   return;

        if (zone.script && ((rand()&255) < zone.chance))
		{
			ExecuteEvent(zone.script);
			timer_count = 0;
		}
    }

    void TestActivate(Entity& player)
    {
        int tx = player.x/16;
        int ty = player.y/16;
        Entity::TweakTileCoords(tx, ty, player.direction);

        int e = EntityAt(tx, ty);
        if (e!=-1 && !ents[e].adjactivate)
        {
           ExecuteEvent(ents[e].actscript);
		   timer_count = 0;

           input.UnPress(1);
           return;
        }

        int z = map->Zone().Get(tx, ty);
        MapFile::ZoneInfo& zone = map->GetZoneInfo(z);
        if (zone.adjactivate)
        {
            ExecuteEvent(zone.script);
			timer_count = 0;
            input.UnPress(1);
            return;
        }
    }

    void HandlePlayer(Entity& player)
    {
        if (player.movecount)
            player.Update();            // just keep on going.  We're doing tile movement here.

        if (player.movecount==0)
        {
            if (player.ismoving)        // so that the event is only checked once; when the entity first steps on the tile.  Once it's executed, the entity can stand there forever, and the event won't execute.
                CheckZones(player);

            if (input.up)           MovePlayer(player, up);
            else if (input.down)    MovePlayer(player, down);
            else if (input.left)    MovePlayer(player, left);
            else if (input.right)   MovePlayer(player, right);
            else                    player.Stop();

            if (input.b1)
                TestActivate(player);

            player.Update();
        }

    }

};
