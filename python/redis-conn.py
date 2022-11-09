from redis import Redis

client = Redis()
if client.ping() is True:
  print('OK')
else:
  print('No')

class Cache:
  def __init__(self, client):
    self.client = client

  def set(self, key, val):
    self.client.set(key,val)

  def get(self, key):
    return self.client.get(key)

  def update(self, key, val):
    return self.client.getset(key, val)



def main():
  cache = Cache(client)
  cache.set('key', 'val')
  val = cache.get('key')
  print(val)
  cache.update('key', 'val2')
  val = cache.get('key')
  print(val)

main()
