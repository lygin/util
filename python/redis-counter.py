from redis import Redis

client = Redis()

class Counter:
  def __init__(self, client, hash_key, counter_name):
    self.client = client
    self.hash_key = hash_key
    self.counter_name = counter_name

  def increase(self, amount=1):
    return self.client.hincrby(self.hash_key, self.counter_name, amount)

  def decrease(self, amount=1):
    return self.client.hincrby(self.hash_key, self.counter_name, -amount)

  def get(self):
    value = self.client.hget(self.hash_key, self.counter_name)
    if value is None:
      return 0;
    return int(value)

  def reset(self):
    return self.client.hset(self.hash_key, self.counter_name, 0)

user_msk_counter = Counter(client, 'page_view_counts', '/home/msk')
print(user_msk_counter.increase())

user_lj_counter = Counter(client, 'page_view_counts', '/home/lj')
print(user_lj_counter.increase())

print(user_msk_counter.increase(10))


