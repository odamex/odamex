import lldb

# load this with:
# command script import path/to/odamex_lldb.py

def OFlagsSummary(valobj, _):
    type = valobj.GetType()
    enum_type = type.GetTemplateArgumentType(0)

    type_name = type.GetName()
    value = valobj.GetChildMemberWithName("m_value").GetValueAsUnsigned()

    if value == 0:
        return "none"

    fields = [
        enum_type.GetEnumMembers().GetTypeEnumMemberAtIndex(i)
        for i in range(enum_type.GetEnumMembers().GetSize())
    ]

    fields = [
        field for field in fields
        if field.GetValueAsUnsigned() != 0
    ]

    sorted_fields = sorted(
        fields,
        key=lambda field: field.GetValueAsUnsigned().bit_count(),
        reverse=True,
    )

    selected = set()
    remaining = value

    for field in sorted_fields:
        flag = field.GetValueAsUnsigned()

        if (remaining & flag) == flag:
            selected.add(field.GetName())
            remaining &= ~flag

    names = [
        field.GetName().rsplit("::", 1)[-1]
        for field in fields
        if field.GetName() in selected
    ]

    if remaining:
        names.append(f"0x{remaining:x}")

    return f"{type_name}({' | '.join(names)})"

def __lldb_init_module(debugger, _):
    debugger.HandleCommand(
        'type summary add -x -F odamex_lldb.OFlagsSummary "flags_detail::flag_(set|combo|mask)<.*>"'
    )