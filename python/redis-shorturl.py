from redis import Redis
from base36 import base10_to_base36

client = Redis()

id_counter = "shorturl.id_counter"
url_hash = "shorturl.url_hash"

class Shorturl:
  def __init__(self, client):
    self.client = client

  def shorten_url(self, url):
    id = self.client.incr(id_counter)
    short_id = base10_to_base36(id)
    self.client.hset(url_hash, short_id, url)
    return short_id

  def get(self, short_url):
    return self.client.hget(url_hash, short_url)

shorturl = Shorturl(client)

print(shorturl.shorten_url("a.com"))
print(shorturl.shorten_url("b.com"))
print(shorturl.shorten_url("c.com"))

print(shorturl.get("1"))
print(shorturl.get("2"))
print(shorturl.get("3"))
