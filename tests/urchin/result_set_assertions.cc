/*
 * Copyright 2015 Cloudius Systems
 */

#include <boost/test/unit_test.hpp>

#include "result_set_assertions.hh"
#include "to_string.hh"

static inline
sstring to_sstring(const bytes& b) {
    return sstring(b.begin(), b.end());
}

bool
row_assertion::matches(const query::result_set_row& row) const {
    for (auto&& column_and_value : _expected_values) {
        auto&& name = column_and_value.first;
        auto&& value = column_and_value.second;

        // FIXME: result_set_row works on sstring column names instead of more general "bytes".
        auto ss_name = to_sstring(name);

        if (!row.has(ss_name)) {
            if (!value.empty()) {
                return false;
            }
        } else {
            const data_value& val = row.get_data_value(ss_name);
            if (val != data_value(boost::any(value), val.type())) {
                return false;
            }
        }
    }
    return true;
}

sstring
row_assertion::describe(schema_ptr schema) const {
    return "{" + ::join(", ", _expected_values | boost::adaptors::transformed([&schema] (auto&& e) {
        auto&& name = e.first;
        auto&& value = e.second;
        const column_definition* def = schema->get_column_definition(name);
        if (!def) {
            BOOST_FAIL(sprint("Schema is missing column definition for '%s'", name));
        }
        if (value.empty()) {
            return sprint("%s=null", to_sstring(name));
        } else {
            return sprint("%s=\"%s\"", to_sstring(name), def->type->to_string(def->type->decompose(value)));
        }
    })) + "}";
}

const result_set_assertions&
result_set_assertions::has(const row_assertion& ra) const {
    for (auto&& row : _rs.rows()) {
        if (ra.matches(row)) {
            return *this;
        }
    }
    BOOST_FAIL(sprint("Row %s not found in %s", ra.describe(_rs.schema()), _rs));
    return *this;
}

const result_set_assertions&
result_set_assertions::has_only(const row_assertion& ra) const {
    BOOST_REQUIRE(_rs.rows().size() == 1);
    auto& row = _rs.rows()[0];
    if (!ra.matches(row)) {
        BOOST_FAIL(sprint("Expected %s but got %s", ra.describe(_rs.schema()), row));
    }
    return *this;
}

const result_set_assertions&
result_set_assertions::is_empty() const {
    BOOST_REQUIRE_EQUAL(_rs.rows().size(), 0);
    return *this;
}

const result_set_assertions&
result_set_assertions::has_size(int row_count) const {
    BOOST_REQUIRE_EQUAL(_rs.rows().size(), row_count);
    return *this;
}
