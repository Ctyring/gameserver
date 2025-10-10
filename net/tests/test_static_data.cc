#include "cfl/static_data.h"
#include <spdlog/spdlog.h>
using namespace cfl;
int main(){
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::string name = "TestName";
    auto constant_data = StaticData::instance().get_constant_value(name);
    spdlog::info("name {} value {}", name, constant_data);
    auto max_value = StaticData::instance().get_action_max_value(1);
    auto unit_time = StaticData::instance().get_action_unit_time(1);
    spdlog::info("action_id {} max_value {} unit_time {}", 1, max_value, unit_time);
    auto carrer_info = StaticData::instance().get_carrer_info(1);
    spdlog::info("carrer_id {} name {} city {}", carrer_info->actor_id, carrer_info->name, carrer_info->born_city);
    auto skill_info = StaticData::instance().get_actor_skill_info(1);
    spdlog::info("actor_id {} skill_id {}", skill_info->actor_id, skill_info->normal_id);
    return 0;
}