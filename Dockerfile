# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build
# Установите отладочные инструменты

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

# Папка data больше не нужна
COPY ./src /app/src 
COPY ./tests /app/tests  
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_FLAGS="-DBOOST_BEAST_USE_STD_STRING_VIEW -DBOOST_ASIO_HAS_STD_STRING_VIEW" .. && \
    cmake --build .
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Экспортируем порт
EXPOSE 8080

# Создадим пользователя www, но не переключаемся на него
# так как это может вызвать проблемы с сетью
RUN groupadd -r www && useradd -r -g www www

# Создаем рабочую директорию
WORKDIR /app

# Скопируем приложение со сборочного контейнера
COPY --from=build /app/build/bin/game_server /app/

# Копируем все необходимые файлы
COPY ./data /app/data
COPY ./static /app/static

# Установим права на файлы
RUN chown -R www:www /app

# Проверяем существование файлов
RUN ls -la /app/ && \
    ls -la /app/data/ && \
    ls -la /app/static/

# Переключаемся на пользователя www при запуске
USER www

# Запускаем игровой сервер с правильными аргументами
ENTRYPOINT ["/app/game_server"]
#ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]
CMD ["--config-file", "/app/data/config.json", "--www-root", "/app/static"]
#CMD ["/app/game_server", "/app/data/config.json", "/app/static"]
