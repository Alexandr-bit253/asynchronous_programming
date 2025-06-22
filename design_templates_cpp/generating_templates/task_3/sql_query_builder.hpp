#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <map>


class SqlSelectQueryBuilder {
private:
	std::vector<std::string> columns;
	std::string table;

protected:
	std::vector<std::string> where_clauses;

public:
	SqlSelectQueryBuilder& AddColumn(const std::string& column) {
		columns.push_back(column);
		return *this;
	}

	SqlSelectQueryBuilder& AddColumns(const std::vector<std::string>& columns) noexcept {
		this->columns.insert(this->columns.end(), columns.begin(), columns.end());
		return *this;
	}

	SqlSelectQueryBuilder& AddFrom(const std::string& table) {
		this->table = table;
		return *this;
	}

	SqlSelectQueryBuilder& AddWhere(const std::string& column, const std::string& value) {
		where_clauses.push_back(column + "=" + value);
		return *this;
	}

	SqlSelectQueryBuilder& AddWhere(const std::map<std::string, std::string>& kv) noexcept {
		for (const auto& [col, val] : kv) {
			where_clauses.push_back(col + "=" + val);
		}
		return *this;
	}

	std::string BuildQuery() const {
		std::ostringstream ss;

		// select
		if (columns.empty()) {
			ss << "SELECT *";
		}
		else {
			ss << "SELECT ";
			for (size_t i = 0; i < columns.size(); ++i) {
				if (i) ss << ", ";
				ss << columns[i];
			}
		}

		// from
		assert(!table.empty() && "table not specified - be sure to call AddFrom()");
		ss << " FROM " << table;

		// where
		if (!where_clauses.empty()) {
			ss << " WHERE ";
			for (size_t i = 0; i < where_clauses.size(); ++i) {
				if (i) ss << " AND ";
				ss << where_clauses[i];
			}
		}

		ss << ";";
		return ss.str();
	}
};


class AdvancedSqlSelectQueryBuilder : public SqlSelectQueryBuilder {
public:
	AdvancedSqlSelectQueryBuilder& AddWhereGreater(
		const std::string& column,
		const std::string& value) 
	{
		where_clauses.push_back(column + ">" + value);
		return *this;
	}

	AdvancedSqlSelectQueryBuilder& AddWhereLess(
		const std::string& column,
		const std::string& value) 
	{
		where_clauses.push_back(column + "<" + value);
		return *this;
	}
};