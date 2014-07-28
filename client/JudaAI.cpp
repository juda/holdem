#include "client.h"
#include "player.h"
#include "common.h"
#include <iostream>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <cstdlib>
using namespace std;

namespace Extra
{
	const int m = 13;
	char str[] = {"23456789TJQKA"};
	vector<int> hash;
	map<int, int> yr;
	int sum=0;
	unsigned int seed=0;
	__inline int getid(int *rank, int flush)
	{
		vector<int> a[6];
		int mark[15];
		memset(mark,0,sizeof mark);
		for (int i = 1; i <= 5; i++)
			a[i].clear();
		for (int i = 0; i < 5; i++)
		{
			int tmp=0;
			for(int j=0;j<5;j++)
			if(rank[i]==rank[j])tmp++;
			mark[rank[i]]=tmp;
		}
		for(int i=0;i<13;i++)if(mark[i])a[mark[i]].push_back(i);
		//for (int i = 1; i <= 5; i++)	printf("%d ", a[i].size());printf("\n");
		//for (int i = 1; i <= 5; i++)
			//sort(a[i].begin(), a[i].end());
		int v = 0x1fffffff;
		bool straight = 1;
		if (a[1].size() != 5)
			straight = 0;
		else
			for (int i = 0; i < 4; i++)
			{
				if(i==3 && a[1][3]==3 && a[1][4]==12)
				{
					a[1][4]=3;
					break;
				}
				if (a[1][i] != a[1][i + 1] - 1)
				{
					straight = 0;
					break;
				}
			}
		if (straight && flush)
			return v + a[1][4];
		v -= 10000000;
		if (a[4].size() == 1)
			return v + m * a[4][0] + a[1][0];
		v -= 10000000;
		if (a[3].size() == 1 && a[2].size() == 1)
			return v + m * a[3][0] + a[2][0];
		v -= 10000000;
		if (flush)
			return v + m * m * m * m * a[1][4] + m * m * m * a[1][3] + m * m * a[1][2] + m * a[1][1] + a[1][0];
		v -= 10000000;
		if (straight)
			return v + a[1][4];
		v -= 10000000;
		if (a[3].size() == 1)
			return v + m * m * a[3][0] + m * a[1][1] + a[1][0];
		v -= 10000000;
		if (a[2].size() == 2)
			return v + m * m * a[2][1] + m * a[2][0] + a[1][0];
		v -= 10000000;
		if (a[2].size() == 1)
			return v + m * m * m * a[2][0] + m * m * a[1][2] + m * a[1][1] + a[1][0];
		v -= 10000000;
		return v + m * m * m * m * a[1][4] + m * m * m * a[1][3] + m * m * a[1][2] + m * a[1][1] + a[1][0];
	}

	void init()
	{
		int a[6];
		for (int f = 0; f < 2; f++)
			for (a[0] = 0; a[0] < 13; a[0]++)
				for (a[1] = a[0] + f; a[1] < 13; a[1]++)
					for (a[2] = a[1] + f; a[2] < 13; a[2]++)
						for (a[3] = a[2] + f; a[3] < 13; a[3]++)
							for (a[4] = a[3] + f; a[4] < 13; a[4]++)
							{
								if (a[0] == a[4])
									continue;
								hash.push_back(getid(a, f));
							}
		sort(hash.begin(), hash.end());
		for (int i = 0; i < hash.size(); i++)
			if (i == 0 || hash[i - 1] != hash[i])
				yr[hash[i]] = ++sum;
	}

	int getRank(hand_type& a)
	{
		int flush=1,b[6],i,j;
		for(i=1;i<5;++i)
		{
			if(a[i].second!=a[i-1].second)
			{
				flush=0;
				break;
			}
		}
		for(i=0;i<5;++i)
		{
			for(j=0;j<m;++j)
			{
				if(a[i].first==str[j])
				{
					b[i]=j;
					break;
				}
			}
		}
		sort(b,b+5);
		return yr[getid(b,flush)];
	}
	
	unsigned int Rand()
	{
		seed=(seed*233+100000007)&0xFFFFFFFF;
		return seed;
	}
}

string Player::login_name()
{
	auto res=string("Juda");
	Extra::seed=time(0);
	res+=(char)(Extra::Rand()%26+'A');
	return res;
}

void Player::login_name(string name)
{
}

void Player::init()
{
	Extra::init();
}

void Player::destroy()
{
}

decision_type Player::preflop()
{
//	auto money=query.chips(query.my_id());
	auto little=query.blind();
	auto now=query.bets().rbegin()->second;
	auto hole=query.hole_cards();
	if(hole[0].first==hole[1].first || hole[0].second==hole[1].second)
	{
		if(now<=little*3)
		{
			return make_decision(CALL);
		}else
		{
			return make_decision(FOLD);
		}
	}
	if(now==little)
	{
		return make_decision(CALL);
	}else
	{
		return make_decision(FOLD);
	}
}

hand_type Player::choose_best(vector<card_type> poker)
{
	hand_type res,best;
	int now=0,n=poker.size(),i1,i2,i3,i4,i5,temp;
	for(i1=0;i1<n;i1++)
	{
		res[0]=poker[i1];
		for(i2=i1+1;i2<n;i2++)
		{
			res[1]=poker[i2];
			for(i3=i2+1;i3<n;i3++)
			{	
				res[2]=poker[i3];
				for(i4=i3+1;i4<n;i4++)
				{
					res[3]=poker[i4];
					for(i5=i4+1;i5<n;i5++)
					{
						res[4]=poker[i5];
						temp=Extra::getRank(res);
						if(temp>now)
						{
							now=temp;
							best=res;
						}
					}
				}
			}
		}
	}
	return best;
}

decision_type Player::flop()
{
	vector<card_type> poker(query.community_cards().begin(),query.community_cards().end());
	poker.push_back(query.hole_cards()[0]);
	poker.push_back(query.hole_cards()[1]);
	hand_type hand=choose_best(poker);
	int credit=Extra::getRank(hand);
	if(credit>=1500)
	{
		if(credit>=3000)
		{
			return make_decision(CALL);
		}else
		{
			return make_decision(CALL);
		}
	}else
	{
		if(Extra::Rand()%100<30)return make_decision(FOLD);else return make_decision(CHECK);
	}
}

decision_type Player::turn()
{
	vector<card_type> poker(query.community_cards().begin(),query.community_cards().end());
	poker.push_back(query.hole_cards()[0]);
	poker.push_back(query.hole_cards()[1]);
	hand_type hand=choose_best(poker);
	int credit=Extra::getRank(hand);
	if(credit>=2000)
	{
		if(credit>=3500)
		{
			return make_decision(CALL);
		}else
		{
			return make_decision(CALL);
		}
	}else
	{
		if(credit<Extra::Rand()%1000+500)return make_decision(FOLD);else return make_decision(CHECK);
	}
}

decision_type Player::river()
{
	vector<card_type> poker(query.community_cards().begin(),query.community_cards().end());
	poker.push_back(query.hole_cards()[0]);
	poker.push_back(query.hole_cards()[1]);
	hand_type hand=choose_best(poker);
	int credit=Extra::getRank(hand);
	if(credit>=2000)
	{
		if(credit>=5000)
		{
			return make_decision(CALL);
		}else
		{
			return make_decision(CALL);
		}
	}else
	{
		if(credit<1500)return make_decision(FOLD);else return make_decision(CHECK);
	}
}

hand_type Player::showdown()
{
	vector<card_type> poker(query.community_cards().begin(),query.community_cards().end());
	poker.push_back(query.hole_cards()[0]);
	poker.push_back(query.hole_cards()[1]);
	return choose_best(poker);
}

void Player::game_end()
{
	//do nothing
}

