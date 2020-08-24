# Розв'язання двомірної задачі теплопровідності з використанням MPI
### Build & Run
```
   $ mkdir build && cd build
   $ cmake -DCMAKE_BUILD_TYPE=Release  -G"Unix Makefiles" ..
   $ make
   
   $ mpirun -np [number] ./mpi
   or
   $ mpirun -np [number] ./mpi ../conf.txt
```

### Щоб створити гіфку використовуємо бібліотеку imageio, запуск :
```
   $ python3 gify.py
```

результат буде записатий в heat.gif

### Файл conf.dat :

можна задати температуру для кожної сторони

left_border 100

top_border 90

right_border 80

bottom_border 70

Приклад як виглядатиме початкова табличка

tttttttr

l000000r

l000000r

lbbbbbbb

час задаємо як ціле число

size_t delta_t;

size_t save;  // інтервал через який зберігати

size_t maxTime;
