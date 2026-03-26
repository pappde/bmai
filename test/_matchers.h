// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai

#pragma once

#include <gtest/gtest.h>
#include "../src/BMC_Game.h"
#include "../src/BMC_Move.h"


inline std::string str(const std::vector<int>& vec) {
	std::ostringstream oss;
	oss << "{";
	for (size_t i = 0; i < vec.size(); ++i) {
		oss << vec[i];
		if (i != vec.size() - 1) {
			oss << ",";
		}
	}
	oss << "}";
	return oss.str();
}

class ActionMatcher
{
public:
	using is_gtest_matcher = void;

	ActionMatcher(BME_ACTION action) : action_(action) {}

	bool MatchAndExplain(const BMC_Move& _move,
		::testing::MatchResultListener* listener) const
	{
		*listener << "move.action==" << c_action_name[_move.m_action];
		return _move.m_action == action_;
	}

	void DescribeTo(::std::ostream* os) const
	{
		*os << "action==" << c_action_name[action_];
	}

	void DescribeNegationTo(::std::ostream* os) const
	{
		*os << "action!=" << c_action_name[action_];
	}

private:
	BME_ACTION action_;
};

class AttackMatcher
{
public:
	using is_gtest_matcher = void;
	AttackMatcher(BME_ATTACK_TYPE type, const std::string& name, std::vector<int> atk_idxs, std::vector<int> tgt_idxs)
		: type_(type), name_(name), atk_idxs_(atk_idxs), tgt_idxs_(tgt_idxs) {}

	bool MatchAndExplain(const BMC_Move& _move,
						 ::testing::MatchResultListener* listener) const
	{


		auto actname = c_action_name[_move.m_action];
		auto type = c_attack_type[_move.m_attack];
		auto atkname = c_attack_name[_move.m_attack];

		auto a_player = _move.m_game->GetPlayer(_move.m_attacker_player);
		std::vector<int> atk_idxs;
		auto a_bits = _move.m_attackers;
		for (int i = 0; i<a_player->GetAvailableDice(); i++)
			if (a_bits[i])
				atk_idxs.push_back(a_player->GetDie(i)->GetOriginalIndex());
		std::sort(atk_idxs.begin(), atk_idxs.end());
		if (atk_idxs.empty())
			atk_idxs.push_back(a_player->GetDie(_move.m_attacker)->GetOriginalIndex());


		auto t_player = _move.m_game->GetPlayer(_move.m_target_player);
		std::vector<int> tgt_idxs;
		auto t_bits = _move.m_targets;
		for (int i = 0; i<t_player->GetAvailableDice(); i++)
			if (t_bits[i])
				tgt_idxs.push_back(t_player->GetDie(i)->GetOriginalIndex());
		std::sort(tgt_idxs.begin(), tgt_idxs.end());
		if (tgt_idxs.empty())
			tgt_idxs.push_back(t_player->GetDie(_move.m_target)->GetOriginalIndex());

		*listener << " action:" << actname << " type:" << type << " name:" << atkname <<" attacker_dice:" << str(atk_idxs) << " target_dice:" << str(tgt_idxs);
		return  _move.m_action == action_ &&
			type == type_ &&
			atkname == name_ &&
			atk_idxs == atk_idxs_ &&
			tgt_idxs == tgt_idxs_;
	}

	void DescribeTo(::std::ostream* os) const
	{
		*os << " action:" << c_action_name[action_] << " type:" << type_ << " name:" << name_ << " attacker_dice:" << str(atk_idxs_) << " target_dice:" << str(tgt_idxs_);
	}

	void DescribeNegationTo(::std::ostream* os) const
	{
		*os << "NOT";
		DescribeTo(os);
	}

private:
	BME_ACTION action_ = BME_ACTION_ATTACK;
	BME_ATTACK_TYPE type_;
	std::string name_;
	std::vector<int> atk_idxs_;
	std::vector<int> tgt_idxs_;
};

inline ::testing::Matcher<const BMC_Move&> IsAttack(BME_ATTACK_TYPE type, const std::string& name, std::initializer_list<int> atk_idx, std::initializer_list<int> tgt_idx)
{
	return AttackMatcher(type, name, atk_idx, tgt_idx);
}

inline ::testing::Matcher<const BMC_Move&> IsAttack(BME_ATTACK_TYPE type, const std::string& name, int atk_idx, int tgt_idx)
{
	return IsAttack(type, name, {atk_idx}, {tgt_idx});
}
inline ::testing::Matcher<const BMC_Move&> IsAttack(BME_ATTACK_TYPE type, const std::string& name, int atk_idx, std::initializer_list<int> tgt_idx)
{
	return IsAttack(type, name, {atk_idx}, tgt_idx);
}
inline ::testing::Matcher<const BMC_Move&> IsAttack(BME_ATTACK_TYPE type, const std::string& name, std::initializer_list<int> atk_idx, int tgt_idx)
{
	return IsAttack(type, name, atk_idx, {tgt_idx});
}

inline ::testing::Matcher<const BMC_Move&> IsAction(BME_ACTION action)
{
	return ActionMatcher(action);
}
