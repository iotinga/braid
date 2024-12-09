# BRAID Backend

Repository for the backend infrastructure of BRAID project

### Zabbix demo setup

1. Open [Zabbix UI](http://localhost/zabbix) and log in
2. Navigate to **Data collection > Hosts**
3. Press the **Import** button on the top right corner of the page
4. Upload the `zabbix/zbx_export_hosts.yaml` file and verify that the hosts show up in the dashboard

### Grafana demo setup

1. Open the [Grafana UI](https://localhost/grafana) and log in
2. Navigate to **Dashboards**
3. Press the **New** button, then **Import**
4. Upload the `grafana/dashboards/demo.json` file, or paste its contents in the text field
5. Press **Load**, then confirm with **Import**

### Keycloak setup

Keycloak actually supports environment variables when importing configuration. For a working example, see `data/keycloak/import/demo-realm.json`, under the configuration object for the `grafana-oauth` client. The object contains the following:

```jsonc
"attributes" : {
    // other attributes
    "frontchannel.logout.url" : "https://${DOMAIN}/grafana/logout",
    // other attributes
}
```

For example, if `$DOMAIN` is set to `braid.tinga.io`, the `frontchannel.logout.url` attribute will be evaluated to `https://braid.tinga.io/grafana/logout` when the container is first created and the realm does not exist yet.

> [!WARNING]
> The configuration will be imported **ONLY IF THE REALM DOES NOT EXIST** in the container. This means that if `$DOMAIN` is changed, either:
>
> - the container must be recreated from scratch
> - the realm must be deleted in the admin UI and the container restarted

To configure and export a realm, the following procedure must be followed:

1. Bring the Docker Compose stack online with `docker compose up`
2. Run `scripts/keycloak_realm_export.sh`
3. The exported configuration will be in `scripts/export`
