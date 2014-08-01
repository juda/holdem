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
        	self.my_id=0
        	self.I_am_out=False
        	self.roundth=0
        	self.ischeck=1

        def clear(self):
        	self.mycards=[]
        	if self.current_money[self.my_id]<=0:
        		self.I_am_out=True
        	else:
        		self.I_am_out=False
        	for i in xrange(len(self.names)):
        		self.current_bets[i]=0

        def recieve(self):
        	res=''
        	while True:
        		temp=info.client.recv(1)
        		if temp=='\n':
        			print res
        			return res
        		res+=temp

        def sent(self,message):
        	print '[[send!]]',message.rstrip()
        	info.client.send(message)
    
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
                	temp=Card.numbers.index(i[0])
                	hand.append(temp)
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
                if flush==1:
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
                print '[card:]',cards
                now=(0,0)
                best=None
                for i1 in xrange(n):
                        for i2 in xrange(i1+1,n):
                                for i3 in xrange(i2+1,n):
                                        for i4 in xrange(i3+1,n):
                                                for i5 in xrange(i4+1,n):
                                                        hand=(cards[i1],cards[i2],cards[i3],cards[i4],cards[i5])
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
    query.sent('login Juda\n')
    print 'login successful!'

def GameInit():
	realname=query.recieve().split()[1]
	print realname
	while True:
		name=query.recieve().rstrip()
		if name=='player list end':
			break
		if name==realname:
			query.my_id=len(query.names)
		query.names.append(name[0])
		query.current_money.append(0)
		query.current_bets.append(0)
		query.current_players.append(0)
	n=len(query.names)
	money=int(query.recieve().split()[3])
	for i in xrange(n):
	    query.current_money[i]=money

def GameBegin():
	query.clear()
	query.roundth+=1
	print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>round %d go!'%query.roundth
	query.number_of_pots=1
	temp=query.recieve().split()
	query.number_of_participants=int(temp[4])
	query.current_players=map(int,query.recieve().split())
	query.blind=int(query.recieve().split()[3])

	query.dealer=int(query.recieve().split()[2])
	members=query.number_of_participants

	command=query.recieve().split()
	location=int(command[1])
	query.current_bets[location]=int(command[4])
	query.current_money[location]-=int(command[4])
	query.current_pot=query.current_bets[location]

	
	command=query.recieve().split()
	location=int(command[1])
	query.current_bets[location]=int(command[4])
	query.current_money[location]-=int(command[4])
	query.current_pot=max(query.current_bets[location],query.current_pot)

	query.up=0
	if not query.I_am_out:
		card1,card2=query.recieve().split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
		card1,card2=query.recieve().split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
	firstTurn()

def firstTurn():
	print '[preflop]'
	query.recieve()
	query.ischeck=0
	members=len(query.current_players)
	for i in xrange(members):
		command=query.recieve().split()
		query.current_money[int(command[1])]=int(command[3])

	while True:
		command=query.recieve().split()
		if command[0]=='action':
			print '[!]',query.current_pot,query.current_bets[query.my_id],query.current_money[query.my_id]
			if query.current_bets[query.my_id]>0:
				temp=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				print '[!!]',temp
				query.sent('bet %d\n'%temp)
			elif query.current_money[query.my_id]==0:
				query.sent('bet 0\n')
			else:
				if query.mycards[0][0]==query.mycards[1][0] or query.mycards[0][1]==query.mycards[1][1]:
					temp=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
					print '[!!]',temp
					query.sent('bet %d\n'%temp)
				else:
					query.sent('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
				if member==query.my_id:
					query.I_am_out=True
			elif command[2]=='bets':
				temp=int(command[3])
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[member]>query.current_pot:
					query.up=query.current_bets[member]-query.current_pot
					query.current_pot=query.current_bets[member]
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		for i in xrange(3):
			card1,card2=query.recieve().split()[2:]
			query.mycards.append('%s%s'%(card1,card2))
		secondTurn()

def secondTurn():
	print '[flop]'
	query.recieve()
	query.ischeck=1
	members=len(query.current_players)
	for i in xrange(members):
		command=query.recieve().split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=query.blind*2
	query.up=0
	for i in xrange(len(query.names)):
		query.current_bets[i]=0

	while True:
		command=query.recieve().split()
		if command[0]=='action':
			if query.ischeck:
				if query.current_money[query.my_id]/10>=query.current_pot:
					query.sent('bet %d\n'%query.current_pot)
				else:
					query.sent('bet 0\n')
			elif query.current_money[query.my_id]==0:
				query.sent('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
					query.sent('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<50:
						query.sent('bet %d\n'%atlease)
					else:
						query.sent('fold\n')
				else:
					query.sent('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
				if member==query.my_id:
					query.I_am_out=True
			elif command[2]=='bets':
				temp=int(command[3])
				if temp>0:
					query.ischeck=0
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[member]>query.current_pot:
					query.up=query.current_bets[member]-query.current_pot
					query.current_pot=query.current_bets[member]
			elif command[2]=='checks':
				pass
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		card1,card2=query.recieve().split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
		thirdTurn()

def thirdTurn():
	print '[turn]'
	query.recieve()
	query.ischeck=1
	members=len(query.current_players)
	for i in xrange(members):
		command=query.recieve().split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=query.blind*2
	query.up=0
	for i in xrange(len(query.names)):
		query.current_bets[i]=0

	while True:
		command=query.recieve().split()
		if command[0]=='action':
			if query.ischeck:
				if query.current_money[query.my_id]/10>=query.current_pot:
					query.sent('bet %d\n'%query.current_pot)
				else:
					query.sent('bet 0\n')
			elif query.current_money[query.my_id]==0:
				query.sent('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
						query.sent('bet %d\n'%random.randint(atlease,query.current_money[query.my_id]))
					else:
						query.sent('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<30:
						query.sent('bet %d\n'%atlease)
					else:
						query.sent('fold\n')
				else:
					query.sent('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
				if member==query.my_id:
					query.I_am_out=True
			elif command[2]=='bets':
				temp=int(command[3])
				if temp>0:
					query.ischeck=0
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[member]>query.current_pot:
					query.up=query.current_bets[member]-query.current_pot
					query.current_pot=query.current_bets[member]
			elif command[2]=='checks':
				pass
		elif command[0]=='pot':
			pass

	if query.number_of_participants==1:
		declareWinner()
	else:
		card1,card2=query.recieve().split()[2:]
		query.mycards.append('%s%s'%(card1,card2))
		forthTurn()

def forthTurn():
	print '[river]'
	query.recieve()
	query.ischeck=1
	members=len(query.current_players)
	for i in xrange(members):
		command=query.recieve().split()
		query.current_money[int(command[1])]=int(command[3])

	query.current_pot=query.blind*2
	query.up=0
	for i in xrange(len(query.names)):
		query.current_bets[i]=0

	while True:
		command=query.recieve().split()
		if command[0]=='action':
			if query.ischeck:
				if query.current_money[query.my_id]/10>=query.current_pot:
					query.sent('bet %d\n'%query.current_pot)
				else:
					query.sent('bet 0\n')
			elif query.current_money[query.my_id]==0:
				query.sent('bet 0\n')
			else:
				card=Card()
				card=card.chooseBest(query.mycards)[0]
				atlease=min(query.current_pot-query.current_bets[query.my_id],query.current_money[query.my_id])
				if card[0]>2:
					if card[0]>6:
						query.sent('bet %d\n'%query.current_money[query.my_id])
					elif card[0]>4:
						atlease=min(query.current_pot-query.current_bets[query.my_id]+query.up,query.current_money[query.my_id])
						query.sent('bet %d\n'%random.randint(atlease,query.current_money[query.my_id]))
					else:
						query.sent('bet %d\n'%atlease)
				elif card[0]==2:
					if random.randint(1,100)<20:
						query.sent('bet %d\n'%atlease)
					else:
						query.sent('fold\n')
				else:
					query.sent('fold\n')
		elif command[0]=='round':
			break
		elif command[0]=='player':
			member=int(command[1])
			if command[2]=='folds':
				query.number_of_participants-=1
				if member==query.my_id:
					query.I_am_out=True
			elif command[2]=='bets':
				temp=int(command[3])
				if temp>0:
					query.ischeck=0
				query.current_money[member]-=temp
				query.current_bets[member]+=temp
				if query.current_bets[member]>query.current_pot:
					query.up=query.current_bets[member]-query.current_pot
					query.current_pot=query.current_bets[member]
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
	query.recieve()
	if not query.I_am_out:
		query.recieve()
		card=Card()
		card=card.chooseBest(query.mycards)[1]
		res=''
		for i in card:
			res+='%c%c '%(i[0],i[1])
		res+='\n'
		print '[showdown] %s'%res.rstrip()
		query.sent(res)
	for i in xrange(query.number_of_participants):
		query.recieve()
	print '[showdown end]'

def declareWinner():
	query.recieve()
	while True:
		command=query.recieve().split()
		if command[0]=='game':
			if query.current_money[query.my_id]<=0:
				query.I_am_out=True
			if command[1]=='over':
				print 'My ID is %d'%query.my_id
				exit(0)
			return
		members=int(command[5])
		for j in xrange(members):
			command=query.recieve().split()
			query.current_money[int(command[1])]+=int(command[3])

if __name__=='__main__':
    if len(sys.argv)!=3:
        raise 'usage: <ip> <por>'
    query=info()
    Connect('Juda')
    GameInit()
    query.recieve()
    while True:
    	GameBegin()

