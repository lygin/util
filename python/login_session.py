import random
from time import time
from hashlib import sha256
from redis import Redis

DEFAULT_TIMEOUT = 60*60*24*30 #one month

SESSION_NOT_LOGIN = 'SESSION_NOT_LOGIN'
SESSION_EXPIRED = 'SESSION_EXPIRED'
SESSION_TOKEN_CORRECT = 'SESSION_TOKEN_CORRECT'
SESSION_TOKEN_INCORECT = 'SESSION_TOKEN_INCORECT'

SESSION_TOKEN_HASH = 'SESSION_TOKEN_HASH'
SESSION_TOKEN_HASH_EXPIRE = 'SESSION_TOKEN_HASH_EXPIRE'

def generate_token():
  '''
  generate random token
  '''
  random_str = str(random.getrandbits(256))
  return sha256(random_str.encode('utf-8')).hexdigest()

class LoginSession:
  def __init__(self, client, user_id):
    self.client = client
    self.user_id = user_id

  def create_token(self, timeout=DEFAULT_TIMEOUT):
    '''
    create a new session
    '''
    user_token = generate_token()
    expire_timestamp = time() + timeout
    self.client.hset(SESSION_TOKEN_HASH, self.user_id, user_token)
    self.client.hset(SESSION_TOKEN_HASH_EXPIRE, self.user_id, expire_timestamp)
    return user_token

  def validate_token(self, input_token):
    '''
    validate the user token
    SESSION_NOT_LOGIN = 0
    SESSION_EXPIRED = 1
    SESSION_TOKEN_CORRECT = 2
    SESSION_TOKEN_INCORECT = 3
    '''
    if self.client.hexists(SESSION_TOKEN_HASH, self.user_id) == 0:
      return SESSION_NOT_LOGIN
    if self.client.hget(SESSION_TOKEN_HASH, self.user_id) != input_token:
      return SESSION_TOKEN_INCORECT
    expire_time = self.client.hget(SESSION_TOKEN_HASH_EXPIRE, self.user_id)
    # expire_time is a string so change to float
    if time() > float(expire_time):
      return SESSION_EXPIRED
    return SESSION_TOKEN_CORRECT

  def destroy(self):
    '''
    destroy the session
    '''
    self.client.hdel(SESSION_TOKEN_HASH, self.user_id)
    self.client.hdel(SESSION_TOKEN_HASH_EXPIRE, self.user_id)

client = Redis(decode_responses=True)
login_session = LoginSession(client, 'msk')
token = login_session.create_token()
print(token)
print(login_session.validate_token("WRONG_TOKEN"))
print(login_session.validate_token(token))
login_session.destroy()
print(login_session.validate_token(token))