from redis import Redis

client = Redis()

class Lock:
  def __init__(self, client, lock_key):
    self.client = client
    self.lock_key = lock_key

  def lock(self):
    res = self.client.set(self.lock_key, 1, nx=True)
    return res == True

  def unlock(self):
    return self.client.delete(self.lock_key) == 1

def main():
  lock = Lock(client, 'lock')
  print(lock.lock())
  print(lock.lock())
  print(lock.unlock())

main()