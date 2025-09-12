import re
import sys
from pathlib import Path

# 类型映射表
TYPE_MAP = {
    "int32": "int32_t",
    "int64": "int64_t",
    "uint32": "uint32_t",
    "uint64": "uint64_t",
    "bool": "bool",
    "string": "std::string",
}

HEADER_TEMPLATE = """#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "cfl/cfl.h"
#include "cfl/db/db.h"
#include "cfl/db/db_mysql.h"

namespace cfl::shm {{

struct {struct_name} : public SharedObject {{
{fields}

    [[nodiscard]]
    bool Save() const {{
        return cfl::db::MySQLUtil::execute_fmt("{struct_name}::Save",
            "REPLACE INTO {table} ({columns}) VALUES ({placeholders});",
            {values}
        ) > 0;
    }}

    [[nodiscard]]
    bool Delete() const {{
        return cfl::db::MySQLUtil::execute_fmt("{struct_name}::Delete",
            "UPDATE {table} SET isdelete = 1 WHERE id = {{}}", roleId
        ) > 0;
    }}
}};

}}
"""

def parse_proto(proto_path: Path):
    with open(proto_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    message_name = None
    fields = []

    for line in lines:
        line = line.strip()
        if not line or line.startswith("syntax") or line.startswith("package"):
            continue
        if line.startswith("message "):
            message_name = line.split()[1].strip("{")
        elif re.match(r"^(repeated\s+)?\w+", line):
            parts = line.replace(";", "").split()
            if parts[0] == "repeated":
                proto_type, name = parts[1], parts[2]
                # 检查注释里是否有固定大小标记
                fixed_size = re.search(r"@fixed\((\d+)\)", line)
                if fixed_size:
                    ctype = f"std::array<{TYPE_MAP.get(proto_type, proto_type)}, {fixed_size.group(1)}>"
                else:
                    ctype = f"std::vector<{TYPE_MAP.get(proto_type, proto_type)}>"
            else:
                proto_type, name = parts[0], parts[1]
                ctype = TYPE_MAP.get(proto_type, proto_type)

            fields.append((ctype, name))

    return message_name, fields


def generate_struct(proto_path: Path, table="player"):
    struct_name, fields = parse_proto(proto_path)

    # 字段声明
    field_decls = "\n".join([f"    {ctype} {name}{{}};" for ctype, name in fields])

    # SQL 映射
    columns = ", ".join([name for _, name in fields])
    placeholders = ", ".join(["{}"] * len(fields))
    values = ", ".join([name for _, name in fields])

    header_code = HEADER_TEMPLATE.format(
        struct_name=struct_name + "Object",
        table=table,
        fields=field_decls,
        columns=columns,
        placeholders=placeholders,
        values=values,
    )

    out_path = proto_path.with_suffix(".h")
    out_path.write_text(header_code, encoding="utf-8")
    print(f"✅ Generated: {out_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: proto2cpp.py <file.proto>")
        sys.exit(1)
    generate_struct(Path(sys.argv[1]))
