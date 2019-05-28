from bs4 import BeautifulSoup
from urllib.request import *
import matplotlib.pyplot as plt

url = "https://thanks.rust-lang.org/rust/all-time/"
page = urlopen(url)
soup = BeautifulSoup(page, "html.parser")
rows = soup.find("table").find("tbody").find_all("tr")

conts = []
for i, row in enumerate(rows):
    if i == 0:
        continue
    cells = row.find_all("td")
    assert len(cells) == 3
    conts.append(int(cells[2].get_text()))

plt.plot(conts)
plt.savefig("res.png")
