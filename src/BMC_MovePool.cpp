///////////////////////////////////////////////////////////////////////////////////////////
// BMC_MovePool.cpp
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// REVISION HISTORY:
// dbl100524 - broke this commented out logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
// BMC_MovePool
///////////////////////////////////////////////////////////////////////////////////////////

/*
// TODO: assert on m_pool_index
// TODO: use bits on m_available
class BMC_MovePool
{
public:
	BMC_MovePool()
	{
		m_pool.resize(128);
		m_available.resize(128, true);
		m_next_available = 0;
	}

	BMC_Move *			New()
	{
		CheckSize();
		while (!m_available[m_next_available])
		{
			m_next_available++;
			CheckSize();
		}

		m_available[m_next_available] = false;
		m_pool[m_next_available].m_pool_index = m_next_available;
		return &(m_pool[m_next_available++]);
	}

	void				Release(BMC_Move *_move)
	{
		if (_move->m_pool_index<0)
			return;
		m_available[_move->m_pool_index] = true;
		_move->m_pool_index = -1;
	}


private:

	void	CheckSize()
	{
		BM_ASSERT(m_available.size()==m_pool.size());
		if (m_next_available>=m_available.size())
		{
			m_next_available = 0;
			INT grow = m_available.size() * 2;
			m_available.resize( grow );
			m_pool.resize( grow );
		}
	}

	std::vector<BMC_Move>		m_pool;
	std::vector<bool>	m_available;
	INT					m_next_available;
};

BMC_MovePool	g_move_pool;
*/
