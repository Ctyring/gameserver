#pragma once

#include <cstdint>
#include <set>
#include <memory>
#include "module_base.h"
namespace cfl {
    ModuleBase::ModuleBase(PlayerObjPtr pOwner)
    {
        owner_player = pOwner;
    }

    bool ModuleBase::calc_fight_value(
            int32_t value[PropertyNum],
            int32_t percent[PropertyNum],
            int32_t &fight_value)
    {
        return true;
    }

    std::int64_t ModuleBase::get_property(RoleProperty property_id)
    {
        return 0;
    }

    bool ModuleBase::add_change_id(std::uint64_t id)
    {
        change_set.insert(id);

        return TRUE;
    }

    bool ModuleBase::add_remove_id(std::uint64_t id)
    {
        remove_set.insert(id);

        return TRUE;
    }

    bool ModuleBase::set_owner(PlayerObjPtr owner)
    {
        owner_player = owner;
        return TRUE;
    }

    ModuleBase::PlayerObjPtr ModuleBase::get_owner()
    {
        return owner_player;
    }
} // namespace cfl
