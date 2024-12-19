import pandas as pd
import matplotlib.pyplot as plt

# Загрузка данных из файла
data = pd.read_csv('../statistics/statistics.txt', header=None, delim_whitespace=True)

# Извлечение значений
rsrp = data[0]  # Первое значение в каждой строке
rsrq = data[1]  # Второе значение в каждой строке
snr = data[2]   # Третье значение в каждой строке

# Удаление дубликатов
unique_rsrp = rsrp.unique()
unique_rsrq = rsrq.unique()
unique_snr = snr.unique()

# Вывод уникальных значений
print('Уникальные значения RSRP:')
print(unique_rsrp)
print('Уникальные значения RSRQ:')
print(unique_rsrq)
print('Уникальные значения SNR:')
print(unique_snr)

# Построение графиков
plt.figure(10, figsize=(10,10))
plt.subplot(2, 2, 1)
plt.plot(rsrp)
plt.title("RSRP")
plt.xlabel("Index")
plt.ylabel("RSRP Value")

plt.subplot(2, 2, 2)
plt.plot(rsrq)
plt.title("RSRQ")
plt.xlabel("Index")
plt.ylabel("RSRQ Value")


plt.subplot(2, 2, 3)
plt.plot(snr)
plt.title("SNR")
plt.xlabel("Index")
plt.ylabel("SNR Value")
plt.show()
