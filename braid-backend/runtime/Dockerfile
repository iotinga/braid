FROM eclipse-temurin:21-jdk-jammy AS deps
WORKDIR /build
COPY --chmod=0755 mvnw mvnw
COPY .mvn/ .mvn/
COPY private/ private/
RUN --mount=type=bind,source=pom.xml,target=pom.xml \
    --mount=type=bind,source=application-model/pom.xml,target=application-model/pom.xml \
    --mount=type=bind,source=belt-guijfx/pom.xml,target=belt-guijfx/pom.xml \
    --mount=type=bind,source=core/pom.xml,target=core/pom.xml \
    --mount=type=bind,source=entity-gadget/pom.xml,target=entity-gadget/pom.xml \
    --mount=type=bind,source=etl-gadget/pom.xml,target=etl-gadget/pom.xml \
    --mount=type=bind,source=opcuademo-gadget/pom.xml,target=opcuademo-gadget/pom.xml \
    --mount=type=bind,source=runtime/pom.xml,target=runtime/pom.xml \
    --mount=type=cache,target=/root/.m2 \
    ./mvnw dependency:go-offline -DskipTests

FROM deps AS package
WORKDIR /build
COPY core /build/core
COPY application-model /build/application-model
COPY belt-guijfx /build/belt-guijfx
COPY entity-gadget /build/entity-gadget
COPY etl-gadget /build/etl-gadget
COPY opcuademo-gadget /build/opcuademo-gadget
COPY runtime /build/runtime
RUN --mount=type=bind,source=pom.xml,target=pom.xml \
    --mount=type=cache,target=/root/.m2 \
    ./mvnw package -DskipTests

FROM eclipse-temurin:21-jre-jammy AS user-creation
ARG UID=10001
RUN adduser \
    --disabled-password \
    --gecos "" \
    --home "/nonexistent" \
    --shell "/sbin/nologin" \
    --no-create-home \
    --uid "${UID}" \
    appuser
USER appuser

FROM user-creation AS runtime-service
WORKDIR /work
COPY --from=package /build/runtime/target/runtime-*-jar-with-dependencies.jar /work/runtime.jar
ENTRYPOINT ["java", "-jar", "-Dlog4j2.configurationFile=/etc/braid/log4j2.xml", "/work/runtime.jar"]
CMD ["--help"]
