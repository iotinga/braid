![Finanziato dall'Unione europea | Ministero dell'Università e della Ricerca | Italia domani PNRR | iNEST ](assets/HEADER_INEST.png)

# BRAID

> Nell'ambito dell'agri-food digitale, il progetto BRAID mira a rivoluzionare la gestione della supply chain attraverso l'introduzione di una piattaforma basata sulla blockchain. Il suo fulcro è lo sviluppo e l'implementazione di tecnologie avanzate per una supply chain sostenibile, con enfasi particolare sulla gestione, integrazione e autenticazione di dati eterogenei.
> Al centro della nostra innovazione, progettiamo un prototipo di tracker GPS/LTE arricchito con sensori MEMS capaci di rilevare temperatura, luminosità, umidità, qualità dell'aria e movimenti attraverso una piattaforma inerziale a 6 assi. Questo dispositivo non solo monitorerà le condizioni dei prodotti trasportati, ma garantirà anche l'integrità e l'autenticità dei dati generati, sfruttando la natura immutabile della blockchain.
> La sfida principale è la gestione efficiente di volumi elevati di dati, assicurando la loro consultabilità e minimizzando le risorse richieste per la creazione e registrazione. Questo è cruciale per mantenere la sostenibilità del sistema in un settore dove le risorse sono essenziali. Il nostro focus specifico è il trasporto di vino, con una particolare attenzione al monitoraggio e alla gestione di una confezione da sei bottiglie di vino, garantendo che ogni fase del loro viaggio rispetti gli standard ottimali per la conservazione e l'autenticità del prodotto.

# Architettura della soluzione
Per evidenziare benefici e limiti della soluzione basata su blockchain è stata definita una architettura generale del sistema allo stato dell'arte che supporta sia un modello di trust basato su Certification Authorities sia basato su Block Chain.
Nei paragrafi seguenti vengono illustrati in prima istanza gli elementi comuni alle due architetture. Successivamente viene illustrato il dettaglio di "verifica di autenticità ed inalterabilità", declinata nelle due varianti.

![Architecture Diagram](assets/BRAID-final-X-SCENARIO.drawio.png)

Nel diagramma è possibile identificare gli attori coinvolti nella disamina:
* Dispositivo hardware, costituito da sezione sensori e sezione master.
* Backbone MQTT di raccolta e distribuzione dati
* Identities service per la gestione centralizzata delle capability dei diversi attori aventi accesso alla backbone
* ETL agente software per la trascrizione dei dati in un database relazionale al fine di ottimizzare la fruizione delle informazioni.

## Variante basata su Certification Authority

![Architecture Diagram with Certification Authority](assets/BRAID-final-CA-SCENARIO.drawio.png)

## Variante basata su Block Chain

![Architecture Diagram with Certification Authority](assets/BRAID-final-BC-SCENARIO.drawio.png)