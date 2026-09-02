import gdb


class OFlagsPrinter:
    def __init__(self, value: gdb.Value):
        self.value = value
        self.type = value.type.strip_typedefs()
        self.enum_type = self.type.template_argument(0)
        if self.type.tag is not None and self.type.tag.startswith("flags_detail::flag_set"):
            self.type_name = f"OFlags<{self.enum_type}>"
        else:
            self.type_name = self.type.name

    def to_string(self):
        value = int(self.value["m_value"])

        if value == 0:
            return "none"

        # a few enums have things like MF_TRANSLATION that are not just a single bit
        # so lets go through and check those first before any other fields, then fix the ordering afterwards
        # so that we don't list all 3 of MF_TRANSLATION, MF_TRANSLATION1, and MF_TRANSLATION2
        fields = [
            field
            for field in self.enum_type.fields()
            if field.enumval != 0
        ]

        sorted_fields = sorted(
            fields,
            key=lambda field: int(field.enumval).bit_count(),
            reverse=True,
        )

        selected = set()
        remaining = value

        for field in sorted_fields:
            flag = int(field.enumval)

            if (remaining & flag) == flag and field.name is not None:
                selected.add(field)
                remaining &= ~flag

        names = [
            # the is not none check is just to satisfy type checkers
            field.name.split("::")[-1] for field in fields if field in selected and field.name is not None
        ]

        if remaining:
            names.append(f"0x{remaining:x}")

        return f"{self.type_name}({' | '.join(names)})"

def lookup_oflags(value: gdb.Value):
    type = value.type.strip_typedefs()

    if type.code != gdb.TYPE_CODE_STRUCT or type.tag is None:
        return None

    if type.tag.startswith("flags_detail::flag_"):
        return OFlagsPrinter(value)

    return None

gdb.pretty_printers.append(lookup_oflags)