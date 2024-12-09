FROM timescale/timescaledb:latest-pg16

# When the PostgreSQL container is started, any scripts in the /docker-entrypoint-initdb.d/ directory will be executed in alphabetical order
COPY ./timescaledb/000_init.sh /docker-entrypoint-initdb.d/
RUN chmod +x /docker-entrypoint-initdb.d/000_init.sh
