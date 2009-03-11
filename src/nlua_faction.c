/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file nlua_faction.c
 *
 * @brief Handles the Lua faction bindings.
 *
 * These bindings control the factions.
 */

#include "nlua_faction.h"

#include "lauxlib.h"

#include "log.h"
#include "naev.h"
#include "nlua.h"
#include "nluadef.h"
#include "faction.h"


/*
 * Prototypes
 */
static int factionL_createmetatable( lua_State *L, int readonly );

/* Faction metatable methods */
static int factionL_eq( lua_State *L );
static int factionL_name( lua_State *L );
static int factionL_longname( lua_State *L );
static int factionL_areenemies( lua_State *L );
static int factionL_areallies( lua_State *L );
static int factionL_modplayer( lua_State *L );
static int factionL_modplayerraw( lua_State *L );
static int factionL_playerstanding( lua_State *L );
static int factionL_enemies( lua_State *L );
static int factionL_allies( lua_State *L );
static const luaL_reg faction_methods[] = {
   { "__eq", factionL_eq },
   { "__tostring", factionL_name },
   { "name", factionL_name },
   { "longname", factionL_longname },
   { "areEnemies", factionL_areenemies },
   { "areAllies", factionL_areallies },
   { "modPlayer", factionL_modplayer },
   { "modPlayerRaw", factionL_modplayerraw },
   { "playerStanding", factionL_playerstanding },
   { "enemies", factionL_enemies },
   { "allies", factionL_allies },
   {0,0}
}; /**< Faction metatable methods. */
static const luaL_reg faction_methods_cond[] = {
   { "__eq", factionL_eq },
   { "__tostring", factionL_name },
   { "name", factionL_name },
   { "longname", factionL_longname },
   { "areEnemies", factionL_areenemies },
   { "areAllies", factionL_areallies },
   { "playerStanding", factionL_playerstanding },
   { "enemies", factionL_enemies },
   { "allies", factionL_allies },
   {0,0}
}; /**< Factions read only metatable methods. */


/* Global faction functions. */
static int factionL_get( lua_State *L );
static const luaL_reg factionL_methods[] = {
   { "get", factionL_get },
   {0,0}
}; /**< Faction module functions. */


/**
 * @brief Loads the faction library.
 *
 *    @param L State to load faction library into.
 *    @param readonly Load as read only?
 *    @return 0 on success.
 */
int lua_loadFaction( lua_State *L, int readonly )
{
   /* Registers the faction functions. */
   luaL_register(L, "faction", factionL_methods);

   /* Register the metatables. */
   factionL_createmetatable( L, readonly );

   return 0;
}


/**
 * @brief Registers the faction metatable.
 *
 *    @param L Lua state to register metatable in.
 *    @param readonly Whether table should be readonly.
 *    @return 0 on success.
 */
static int factionL_createmetatable( lua_State *L, int readonly )
{
   /* Create the metatable */
   luaL_newmetatable(L, FACTION_METATABLE);

   /* Create the access table */
   lua_pushvalue(L,-1);
   lua_setfield(L,-2,"__index");

   /* Register the values */
   luaL_register(L, NULL, (readonly) ? faction_methods_cond : faction_methods);

   /* Clean up. */
   lua_pop(L,1);

   return 0; /* No error */
}


/**
 * @defgroup FACTION Faction Lua bindings.
 *
 * @brief Lua bindings to deal with factions.
 * @luamod faction
 * Use with:
 * @code
 * faction.func( params )
 * @endcode
 *
 * @{
 */
/**
 * @brief faction get( string name )
 *
 * Gets the faction based on it's name.
 *
 *    @param name Name of the faction to get.
 *    @return The faction matching name.
 */
static int factionL_get( lua_State *L )
{
   LuaFaction f;
   char *name;

   if (lua_isstring(L,1)) name = (char*) lua_tostring(L,1);
   else NLUA_INVALID_PARAMETER();

   f.f = faction_get(name);
   if (f.f < 0)
      return 0;
   lua_pushfaction(L,f);
   return 1;
}
/**
 * @}
 */

/**
 * @defgroup META_FACTION Faction Metatable
 *
 * @brief The faction metatable is a way to represent a faction in Lua.
 *
 * It allows all sorts of operators making it much more natural to use.
 *
 * To call members of the metatable always use:
 * @code 
 * faction:function( param )
 * @endcode
 *
 * @{
 */
/**
 * @brief Gets faction at index.
 *
 *    @param L Lua state to get faction from.
 *    @param ind Index position to find the faction.
 *    @return Faction found at the index in the state.
 */
LuaFaction* lua_tofaction( lua_State *L, int ind )
{
   if (lua_isuserdata(L,ind)) {
      return (LuaFaction*) lua_touserdata(L,ind);
   }
   luaL_typerror(L, ind, FACTION_METATABLE);
   return NULL;
}
/**
 * @brief Pushes a faction on the stack.
 *
 *    @param L Lua state to push faction into.
 *    @param faction Faction to push.
 *    @return Newly pushed faction.
 */
LuaFaction* lua_pushfaction( lua_State *L, LuaFaction faction )
{
   LuaFaction *f;
   f = (LuaFaction*) lua_newuserdata(L, sizeof(LuaFaction));
   *f = faction;
   luaL_getmetatable(L, FACTION_METATABLE);
   lua_setmetatable(L, -2);
   return f;
}
/**
 * @brief Checks to see if ind is a faction.
 *
 *    @param L Lua state to check.
 *    @param ind Index position to check.
 *    @return 1 if ind is a faction.
 */
int lua_isfaction( lua_State *L, int ind )
{
   int ret;

   if (lua_getmetatable(L,ind)==0)
      return 0;
   lua_getfield(L, LUA_REGISTRYINDEX, FACTION_METATABLE);

   ret = 0;
   if (lua_rawequal(L, -1, -2))  /* does it have the correct mt? */ 
      ret = 1;

   lua_pop(L, 2);  /* remove both metatables */ 
   return ret;
}

/**
 * @brief __eq (equality) metamethod for factions.
 * You can use the '=' operator within Lua to compare factions with this.
 *    @luaparam comp faction to compare against.
 *    @return true if both factions are the same.
 * @luafunc __eq( comp )
 */
static int factionL_eq( lua_State *L )
{
   LuaFaction *a, *b;
   a = lua_tofaction(L,1);
   b = lua_tofaction(L,2);
   lua_pushboolean(L, a->f == b->f);
   return 1;
}

/**
 * @brief Gets the faction's name.
 *    @return The name of the faction.
 * @luafunc name()
 */
static int factionL_name( lua_State *L )
{
   LuaFaction *f;
   f = lua_tofaction(L,1);
   lua_pushstring(L, faction_name(f->f));
   return 1;
}

/**
 * @brief Gets the faction's long name.
 *    @return The long name of the faction.
 * @luafunc longname()
 */
static int factionL_longname( lua_State *L )
{
   LuaFaction *f;
   f = lua_tofaction(L,1);
   lua_pushstring(L, faction_longname(f->f));
   return 1;
}

/**
 * @brief Checks to see if f is an enemy.
 *    @luaparam f Faction to check if is an enemy.
 *    @return true if they are enemies, false if they aren't.
 * @luafunc areEnemies( f )
 */
static int factionL_areenemies( lua_State *L )
{
   LuaFaction *f, *ff;
   f = lua_tofaction(L,1);
   if (lua_isfaction(L,2)) ff = lua_tofaction(L,2);
   else NLUA_INVALID_PARAMETER();

   lua_pushboolean(L, areEnemies( f->f, ff->f ));
   return 1;
}

/**
 * @brief Checks to see if f is an enemy.
 *    @luaparam f Faction to check if is an enemy.
 *    @return true if they are enemies, false if they aren't.
 * @luafunc areAllies( f )
 */
static int factionL_areallies( lua_State *L )
{
   LuaFaction *f, *ff;
   f = lua_tofaction(L,1);
   if (lua_isfaction(L,2)) ff = lua_tofaction(L,2);
   else NLUA_INVALID_PARAMETER();

   lua_pushboolean(L, areAllies( f->f, ff->f ));
   return 1;
}

/**
 * @brief Modifies the player's standing with the faction.
 *    @luaparam mod The modifier to modify faction by.
 * @luafunc modPlayer( mod )
 */
static int factionL_modplayer( lua_State *L )
{
   LuaFaction *f;
   double n;
   f = lua_tofaction(L,1);

   if (lua_isnumber(L,2)) n = lua_tonumber(L,2);
   else NLUA_INVALID_PARAMETER();

   faction_modPlayer( f->f, n );
   return 0;
}

/**
 * @brief Modifies the player's standing with the faction.
 * Does not affect other faction standings.
 *    @luaparam mod The modifier to modify faction by.
 * @luafunc modPlayerRaw( mod )
 */
static int factionL_modplayerraw( lua_State *L )
{
   LuaFaction *f;
   double n;
   f = lua_tofaction(L,1);

   if (lua_isnumber(L,2)) n = lua_tonumber(L,2);
   else NLUA_INVALID_PARAMETER();

   faction_modPlayerRaw( f->f, n );
   return 0;
}

/**
 * @brief Gets the player's standing with the faction.
 *    @return The value of the standing and the human readable string.
 * @luafunc playerStanding()
 */
static int factionL_playerstanding( lua_State *L )
{
   LuaFaction *f;
   int n;
   f = lua_tofaction(L,1);
   n = faction_getPlayer(f->f);
   lua_pushnumber(L, n);
   lua_pushstring(L, faction_getStanding(n));
   return 1;
}

/**
 * @brief Gets the enemies of the faction.
 *    @return A table containing the enemies of the faction.
 * @luafunc enemies()
 */
static int factionL_enemies( lua_State *L )
{
   int i, n;
   int *factions;
   LuaFaction *f, fe;

   f = lua_tofaction(L,1);

   /* Push the enemies in a table. */
   lua_newtable(L);
   factions = faction_getEnemies( f->f, &n );
   for (i=0; i<n; i++) {
      lua_pushnumber(L, i+1); /* key */
      fe.f = factions[i];
      lua_pushfaction(L, fe); /* value */
      lua_rawset(L, -3);
   }

   return 1;
}

/**
 * @brief Gets the allies of the faction.
 *    @return A table containing the allies of the faction.
 * @luafunc allies()
 */
static int factionL_allies( lua_State *L )
{
   int i, n;
   int *factions;
   LuaFaction *f, fa;

   f = lua_tofaction(L,1);

   /* Push the enemies in a table. */
   lua_newtable(L);
   factions = faction_getAllies( f->f, &n );
   for (i=0; i<n; i++) {
      lua_pushnumber(L, i+1); /* key */
      fa.f = factions[i];
      lua_pushfaction(L, fa); /* value */
      lua_rawset(L, -3);
   }

   return 1;
}

/**
 * @}
 */
