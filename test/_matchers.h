#pragma once

#include <gtest/gtest.h>
#include "../src/BMC_Game.h"
#include "../src/BMC_Move.h"

class AttackMatcher : public ::testing::MatcherInterface<const BMC_Move&>
{
public:
	AttackMatcher(BME_ATTACK_TYPE type, const std::string& name, int atk_idx, int tgt_idx)
		: type_(type), name_(name), atk_idx_(atk_idx), tgt_idx_(tgt_idx) {}

	bool MatchAndExplain(const BMC_Move& _move,
						 ::testing::MatchResultListener* listener) const override
	{

		auto type = c_attack_type[_move.m_attack];
		auto name = c_attack_name[_move.m_attack];
		auto atk_idx = _move.m_game->GetPhasePlayer()->GetDie(_move.m_attacker)->GetOriginalIndex();
		auto tgt_idx = _move.m_game->GetTargetPlayer()->GetDie(_move.m_target)->GetOriginalIndex();

		*listener << " action:" << _move.m_action << " type:" << type << " name:" << name <<" attacker_die:" << atk_idx << " target_die:" << tgt_idx;
		return  _move.m_action == action_ &&
			type == type_ &&
			name == name_ &&
			atk_idx == atk_idx_ &&
			tgt_idx == tgt_idx_;
	}

	void DescribeTo(::std::ostream* os) const override
	{
		*os << " action:" << action_ << " type:" << type_ << " name:" << name_ << " attacker_die:" << atk_idx_ << " target_die:" << tgt_idx_;
	}

private:
	BME_ACTION action_ = BME_ACTION_ATTACK;
	BME_ATTACK_TYPE type_;
	std::string name_;
	int atk_idx_;
	int tgt_idx_;
};

inline ::testing::Matcher<const BMC_Move&> IsAttack(BME_ATTACK_TYPE type, const std::string& name, int atk_idx, int tgt_idx)
{
	return MakeMatcher(new AttackMatcher(type, name, atk_idx, tgt_idx));
}
