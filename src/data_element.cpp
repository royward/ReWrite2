#include "data_element.hpp"

namespace detail {
    inline std::string to_string_visit(const DataUnbound&) {
        return "<unbound>";
    }
    inline std::string to_string_visit(const DataBool& b) {
        return b.value ? "true" : "false";
    }
    inline std::string to_string_visit(const DataInt& i) {
        return std::to_string(i.value);
    }
    inline std::string to_string_visit(const DataChar& c) {
        return std::string(1, c.value);
    }
    inline std::string to_string_visit(const DataList& l) {
        std::string out = "{";
        for (std::size_t i = 0; i < l.value.size(); i++) {
            if (i != 0) out += ",";
            out += l.value[i].to_string();
        }
        out += "}";
        return out;
    }
}

std::string DataElement::to_string() const {
    return std::visit([](const auto& alt) { return detail::to_string_visit(alt); }, value);
}
