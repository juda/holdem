import sys
import socket
import random

class info:
        client=None
        def __init__(self):
        	self.current_bets=[]
        	self.current_players=[]
        	self.mycards=[]
        	self.current_money=[]
        	self.names=[]

        def clear(self):
        	self.mycards=[]
        	for i in xrange(len(self.names)):
        		self.current_bets[i]=0
        		self.current_players[i]=0
        		self.current_money[i]=0
    
class Card:
        numbers='23456789TJQKA'
        colors='CDHS'
        def calcCards(self,cards):
                hand=[]
                flush=1
                straight=1
                big=0
                five=[0 for i in xrange(13)]
                four=[0 for i in xrange(5)]
                color=None
                for i in cards:
                    hand.append(card.numbers.index(i[0]))
                    if color==None:
                            color=i[1]
                    if color!=i[1]:
                            flush=0
                sorted(hand)
                for i in hand:
                        five[i]+=1
                for i in xrange(12,-1,-1):
                	four[five[i]]=four[five[i]]*15+i
                for i in xrange(1,5):
                	big=hand[i]
                	if hand[i]!=hand[i-1]+1:
						straight=0
                    	if i==4 and hand[4]==12 and hand[0]==0:
                        	straight=1
                        	big=3
                        else:
                        	break
                if straight==1 and flush==1:
                	return (9,big)
                if four[4]>0:
                	return (8,four[4]*100+four[1])
                if four[3]>0  and four[2]>0:
                	return (7,four[3]*100+four[2])
                if fluse==1:
                	return (6,four[1])
                if straight==1:
                	return (5,big)
                if four[3]>0:
                	return (4,four[3]*1000+four[1])
                if four[2]>15:
                	return (3,four[2]*10000+four[1])
                if four[2]>0:
                	return (2,four[2]*1000+four[1])
                return (1,four[1])

        def chooseBest(self,cards):
                n=len(cards)
                now=(0,0)
                best=None
                for i1 in xrange(n):
                        for i2 in xrange(i1+1,n):
                                for i3 in xrange(i2+1,n):
                                        for i4 in xrange(i3+1,n):
                                                for i5 in xrange(i4+1,n):
                                                        hand=(i1,i2,i3,i4,i5)
                                                        score=self.calcCards(hand)
                                                        if score[0]>now[0] or (score[0]==now[0] and score[1]>now[1]):
                                                                now=score
                                                                best=hand
                return (now,best)


def Connect(name):
    info.client=socket.socket()
    ip=sys.argv[1]
    port=int(sys.argv[2])
    try:
    	print 'connecting...'
    	info.client.connect((ip, port))
    except:
    	print 'connect failed'
    	exit(1)
    info.client.send('login Juda\n')
    print 'login successful!'

def GameInit():
	realname=info.client.recv(1024).split()[1]
    while True:
        name=info.client.recv(1024).split()
        print name
        if name==['player','list','end']:
        	break
        if name[0]==realname:
        	query.my_id=len(query.names)
        query.names.append(name[0])
        query.current_money.append(0)
        query.current_bets.append(0)
        query.current_players.append(0)
    n=len(query.names)
    money=int(info.client.recv(1024).split()[3])
    for i in xrange(n):
        query.current_money[i]=money

def GameBegin():
	query.clear()
	query.number_of_pots=2
	info.client.recv(1024)
	query.number_of_participants=int(info.client.recv(1024).split()[4])
	query.current_players=map(int,info.client.recv(1024).split())

	query.blind=int(info.client.recv(1024).split()[3])

	query.dealer=int(info.client.recv(1024).split()[2])
	location=query.current_players.index(query.dealer)
	location+=1
	if location>=members:
		location-=members
	query.current_bets[location]=int(info.client.recv(1024).split()[4])
	query.current_pot=query.current_bets[location]

	location+=1
	if location>=members:
		location-=members
	query.current_bets[location]=int(info.client.recv(1024).split()[4])

	query.current_pot=max(query.current_bets[location],query.current_pot)
	query.up=0
	card1,card2=info.client.recv(1024).split()[2:]
	query.mycards.append('%s%s'%(card1,card2))
	card1,card2=info.client.recv(1024).split()[2:]
	query.mycards.append('%s%s'%(card1,card2))
	firstTurn()

def firstTurn():
	print 'preflop'
	info.client.recv(1024)
	members=len(query.current_players)
	for i in members:
		command=info.client.recv(1024).split()
		print command
		query.current_money[int(command[1])]=int(command[3])

	while True:
		command=info.client.recv(1024).split()
		print command
		if command[0]=='action':
			if info.current_bets[info.my_id]>0:
				tmep=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				info.client.send('bet %d\n'%temp)
			else:
				if query.mycards[0][0]==query.mycards[1][0] or query.mycards[0][1]==query.mycards[1][1]:
					tmep=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
					info.client.send('bet %d\n'%temp)
				else:
					info.client.send('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
			elif command[2]=='bets':
				temp=int(command[3])
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[mycards]>query.current_pot:
					query.up=query.current_bets[mycards]-query.current_pot
					query.current_pot=query.current_bets[mycards]
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		for i in xrange(3):
			card1,card2=info.client.recv(1024).split()[2:]
			query.mycards.append('%s%s'%(card1,card2))
		secondTurn()

def secondTurn():
	info.client.recv(1024)
	members=len(query.current_players)
	for i in members:
		command=info.client.recv(1024).split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=0
	query.up=0
	for i in xrange(members):
		query.current_bets[i]=0

	while True:
		command=info.client.recv(1024).split()
		if command[0]=='action':
			if query.current_pot==0:
				if query.current_money[query.my_id]>0:
					info.client.send('bet 1\n')
				else:
					info.client.send('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
					info.client.send('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<50:
						info.client.send('bet %d\n'%atlease)
					else:
						info.client.send('fold\n')
				else:
					info.client.send('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
			elif command[2]=='bets':
				if query.current_pot==0:
					query.number_of_pots+=1
				temp=int(command[3])
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[mycards]>query.current_pot:
					query.up=query.current_bets[mycards]-query.current_pot
					query.current_pot=query.current_bets[mycards]
			elif command[2]=='checks':
				pass
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		card1,card2=info.client.recv(1024).split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
		thirdTurn()

def thirdTurn():
	info.client.recv(1024)
	members=len(query.current_players)
	for i in members:
		command=info.client.recv(1024).split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=0
	query.up=0
	for i in xrange(members):
		query.current_bets[i]=0

	while True:
		command=info.client.recv(1024).split()
		if command[0]=='action':
			if query.current_pot==0:
				if query.current_money[query.my_id]>0:
					info.client.send('bet 1\n')
				else:
					info.client.send('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
						info.client.send('bet %d\n'%random.randint(atlease,query.current_money[query.my_id]))
					else:
						info.client.send('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<30:
						info.client.send('bet %d\n'%atlease)
					else:
						info.client.send('fold\n')
				else:
					info.client.send('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
			elif command[2]=='bets':
				if query.current_pot==0:
					query.number_of_pots+=1
				temp=int(command[3])
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[mycards]>query.current_pot:
					query.up=query.current_bets[mycards]-query.current_pot
					query.current_pot=query.current_bets[mycards]
			elif command[2]=='checks':
				pass
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		card1,card2=info.client.recv(1024).split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
		forthTurn()

def forthTurn():
	info.client.recv(1024)
	members=len(query.current_players)
	for i in members:
		command=info.client.recv(1024).split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=0
	query.up=0
	for i in xrange(members):
		query.current_bets[i]=0

	while True:
		command=info.client.recv(1024).split()
		if command[0]=='action':
			if query.current_pot==0:
				if query.current_money[query.my_id]>0:
					info.client.send('bet 1\n')
				else:
					info.client.send('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>6:
						info.client.send('bet %d\n'%query.current_money[query.my_id])
					elif card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
						info.client.send('bet %d\n'%random.randint(atlease,query.current_money[query.my_id]))
					else:
						info.client.send('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<20:
						info.client.send('bet %d\n'%atlease)
					else:
						info.client.send('fold\n')
				else:
					info.client.send('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
			elif command[2]=='bets':
				if query.current_pot==0:
					query.number_of_pots+=1
				temp=int(command[3])
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[mycards]>query.current_pot:
					query.up=query.current_bets[mycards]-query.current_pot
					query.current_pot=query.current_bets[mycards]
			elif command[2]=='checks':
				pass
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		showdown()
		declareWinner()

def showdown():
	info.client.recv(1024)
	info.client.recv(1024)
	card=Card()
	card=card.chooseBest(query.mycards)[1]
	res=''
	for i in card:
		res+='%c %c'%(i[0],i[1])
	res+='\n'
	info.client.send(res)
	for i in query.number_of_participants:
		info.client.recv(1024)

def declareWinner():
	for i in query.number_of_pots:
		members=int(info.client.recv(1024).split()[5])
		for j in xrange(members):
			command=info.client.recv(1024).split()
			query.current_money[int(command[1])]+=int(command[3])

if __name__=='__main__':
    if len(sys.argv)!=3:
        raise 'usage: <ip> <por>'
    query=info()
    Connect('Juda')
    GameInit()
    while info.client.recv(1024)!='Game Over':
    	GameBegin()

