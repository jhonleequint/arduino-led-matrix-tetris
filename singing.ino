// ============================================================
// NOTES (Hz)
// ============================================================
#define BUZZ_IN 12

#define si  247 
// treble clef, C4 octave (note names in solfège)
#define Do  262 
#define Do_diez 277
#define Re  294
#define Re_diez 311
#define Mi  330
#define Fa  349
#define Fa_diez 370
#define Sol  392 
#define Sol_diez 415
#define La  440
#define La_diez 466
#define Si  494


/*int note[]={
  si, 8, Mi, 12, Fa_diez, 4, Sol, 8, La, 8,
  Si, 8, Mi, 8, Mi, 8, DO, 12, Si, 4, La, 8, La, 8,
  RE, 12, DO, 4, Si, 8, Si, 8,
  MI, 12, RE, 4, DO, 4, Si,4, La, 8,
  Si, 8, Sol, 16, La, 8, Si, 12, La, 4, Sol, 8, Fa_diez, 8,
  Mi, 12, Fa_diez, 4, Sol, 8, La, 8, Si, 8, La, 4, Sol, 4, Fa_diez, 8, Sol, 8, Mi, 24
};*/

// ============================================================
// SOUNDS (note, duration in 100 ms units)
// ============================================================
int move_sound[]= {
  Si, 1
};

int lose_sound[]= {
  Re, 4, Do, 8, si, 8
};

void play_move_sound(){
  tone(BUZZ_IN, move_sound[0],100*move_sound[1]); 
  // no tone?
}

bool notePlaying= false;
short int i= 0, last_delay= 0;
long long int curr_note_time=0;
long long int prev_note_time=0;
void play_lose_sound(){
  curr_note_time= millis();
  if(prev_note_time == 0)
    prev_note_time= curr_note_time;
  
  if(prev_note_time + last_delay > curr_note_time){
    if(!notePlaying)  noTone(BUZZ_IN);
    return;
  }

  if(i == -1){
    noTone(BUZZ_IN);
    return;
  }

  if(notePlaying){
    last_delay= 75* lose_sound[i*2+1];
    notePlaying= false;
    return;
  }
  //else
  notePlaying= true;
  tone(BUZZ_IN, lose_sound[i*2]);
  last_delay= 100* lose_sound[i*2+1];
  i++;
  if(i > sizeof(lose_sound) / sizeof(lose_sound[0] / 2)) i= -1;
}