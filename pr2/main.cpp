#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <random>
#include <iomanip>

#define NUM_ROUNDS 4
using namespace std;
void* drawCards(void* param);
static const int DECKSIZE = 52;
enum Suit {SPADE = 0, CLUB = 1, HEART = 2, DIAMOND = 3};

struct Card{

  int num;
  enum Suit suit;
};

class Dealer{

  static const int DECKSIZE = 52;
  Card hand;

public:
  Dealer();
  Card deal() { return hand; };
  void draw(Card card) { hand = card; };
};

class Deck{
  static const int DECKSIZE = 52;
  int size;
  Card *cards = new Card[DECKSIZE];

public: 
  Deck();
  void shuffle();
  void draw();
  void discard(Card);
  Card topCard();
  void showHand();
};

class Player{
private:
  int num;
  int FULL = 2;
  int handSize;
  Card* hand;
  bool turn;
  bool winner;

public: 
  Player(int);
  void draw(Card);
  void push(Card);
  Card discard();
  bool isPair();
  bool isWinner();
  bool isTurn();
  string getHand();
  void exit();
  void log(string);
  void out();
};

struct thread_data{
  int player_id;
  Card card;
};

Deck deck;
bool winner;
ofstream logfile;
pthread_mutex_t player_mut;
pthread_cond_t winnerExists;
pthread_t dlr, player1, player2, player3, player4;


void init(){
  deck = Deck();
  winner = false;
  logfile.open("log.data", ios::out | ios::app);
}

void *drawCards(void *param){
  auto *arg = (thread_data *)param;
  Player player(arg->player_id);
  player.push(arg->card);

  while(!winner){
    pthread_mutex_lock(&player_mut);
    player.draw(deck.topCard());
    deck.draw();

    if(player.isWinner()){
      winner = true;
      player.exit();

      pthread_cond_signal(&winnerExists);
    }else{
      deck.discard(player.discard());
    }
    pthread_mutex_unlock(&player_mut);

    sleep(1);
  }
  player.exit();
  pthread_mutex_unlock(&player_mut);
}

void *_dealer(void *param){
  auto *arg = (thread_data *)param;
  auto myID = arg->player_id;

  pthread_mutex_lock(&player_mut);
  while(!winner){
    pthread_cond_wait(&winnerExists, &player_mut);
    cout << "Winner exists.\n";
  }
  pthread_mutex_unlock(&player_mut);
}

int main(int argc, char *argv[]){
  for(int i = 0; i < NUM_ROUNDS; i++){
    string _round = "\n----------------------------------------------\n"
                    "\t\tROUND: " + to_string(i+1) + "\n"
                    "----------------------------------------------\n";
    cout << _round;

    init();
    deck.shuffle();
    deck.showHand();
    Dealer dealer;

    thread_data d;
    d.player_id = 5;

    thread_data td[4];
    for(int i = 0; i < 4; i++){
      td[i].player_id = i + 1;
      dealer.draw(deck.topCard());
      td[i].card = dealer.deal();
      deck.draw();
    }
    deck.showHand();

    pthread_mutex_init(&player_mut, nullptr);
    pthread_cond_init(&winnerExists, nullptr);

    pthread_create(&dlr, nullptr, _dealer, (void*) &d);
    pthread_create(&player1, nullptr, drawCards, (void*) &td[0]);
    pthread_create(&player2, nullptr, drawCards, (void*) &td[1]);
    pthread_create(&player3, nullptr, drawCards, (void*) &td[2]);
    pthread_create(&player4, nullptr, drawCards, (void*) &td[3]);

    pthread_join(player1, nullptr);
    pthread_join(player2, nullptr);
    pthread_join(player3, nullptr);
    pthread_join(player4, nullptr);
  }

  logfile.close();

  pthread_mutex_destroy(&player_mut);
  pthread_exit(nullptr);

}

// void* threadDealer(void* par){

//   pthread_exit(NULL);
// }
// void* threadPlayer(void* par){

//   pthread_exit(NULL);
// }
Deck::Deck() : size(DECKSIZE-1){
  srand(time(NULL));
  int count = 0;
  Suit suit;
  for(int i = 0; i < 4; i++){
    switch(i){
      case 0: {
        suit = SPADE;
        break;
      }
      case 1:{
        suit = CLUB;
        break;
      }
      case 2: {
        suit = HEART;
        break;
      }
      case 3: {
        suit = DIAMOND;
        break;
      }
      default: cerr << "invalid suit";
    }
    for(int j = 1; j < 14; j++){
      cards[count].num = j;
      cards[count].suit = suit;
      count++;
    }
  }
}
void Deck::shuffle(){
  Card *temp = new Card[DECKSIZE-1];
  int tempSz = 0;
  bool shuffled = false;
  int j = 0;
  
  while(!shuffled){
    int r = 0;
    while (r < 1 || r > DECKSIZE)
      r = rand() % DECKSIZE + 1;
    Card c = cards[r];
    bool contains = false;
    for(int i = 0; i < tempSz; i++){
      if(temp[i].num == c.num && temp[i].suit == c.suit){
        contains = true;
        break;
      }
    }
    if(!contains){
      temp[j] = c;
      j++;
      tempSz++;
      if(j == DECKSIZE-1) shuffled = true;
    }
  }
  delete[] cards;
  cards = temp;
}

void Deck::draw(){
  Card *temp = new Card[DECKSIZE-1];
  for(int i = 0; i < size; i++)
    temp[i] = cards[i+1];
  delete[] cards;
  cards = temp;
  size--;
  }

void Deck::discard(Card c){
  cards[size] = c;
  size++;
  showHand();
}

void Deck::showHand(){
  ofstream edit("log.data", ios::out | ios::app);

  for(int i = 0; i < size; i++){
    if(i == 0){
      cout << "DECK [" << size + 1 << "]: " << setw(3) << cards[i].num;
    }
    else{
      cout << setw(3) << cards[i].num;
    }
  }
  cout << endl;

  if(edit.is_open()){
    for(int i = 0; i < size; i++){
      if(i == 0) {
        edit << "DECK [" << size + 1 << "]: " << setw(3) << cards[i].num;
      }
      else{
        edit << setw(3) << cards[i].num;
      }
    }
      edit << endl;
  }
}

Player::Player(int n):num(n), handSize(0), winner(false){
  hand = new Card[2];
}

void Player::draw(Card card){
  hand[handSize] = card;
  handSize++;
  log(getHand());
}
string Player::getHand(){
  string str;
  for(int i = 0; i < handSize; i++){
    if(i == 0){
      str = to_string(hand[i].num);
    }
    else{
      str += " " + to_string(hand[i].num);
    }
  }
  return "HAND " + str;
}

Card Player::discard(){
  Card *tempHand = new Card[2];
  int i = rand() % DECKSIZE;
  Card remove = hand[i];
  Card keep;
  if(i == 0){
    keep = hand[1];
  }
  else{
    keep = hand[0];
  }
  tempHand[0] = keep;
  delete[] hand;
  hand = tempHand;
  handSize = 1;
  log("discarded " + to_string(remove.num));
  return remove;
}

bool Player::isWinner(){
  if(isPair()){
    winner = true;
    log("WINS");
    out();
    return true;
  }
  else{
    winner = false;
    out();
    return false;
  }
}
bool Player::isPair(){
  return hand[0].num == hand[1].num;
}

void Player::exit(){
  log("exits round");
}

void Player::log(string str){
  ofstream edit("log.data", ios::out | ios::app);
  if(edit.is_open()) edit << "PLAYER " << num << ": " << str << endl;
}

void Player::out(){
  string win = winner ? "yes" : "no";
  cout << "PLAYER " << num << ":\n" << getHand() << "\n" << "WIN " << win << "\n";
  }

// int x = 2;  //placeholder for argument
  // int y = 3;  //placeholder for argument
  // pthread_t dealer[DECKSIZE]; // need to find an array for this
  // pthread_t player[DECKSIZE]; //need to find an array for this
  // int ret;
  // int i = 0;

  
  // ret = pthread_create(&dealer[i], NULL, &threadDealer, &x);
  // if(ret != 0){
  //   cout << "error: thread not created\n";
  //     }
  // ret = pthread_create(&player[i], NULL, &threadPlayer1, &y);
  // if(ret != 0){
  //   cout << "error: thread not created\n";
  //     }