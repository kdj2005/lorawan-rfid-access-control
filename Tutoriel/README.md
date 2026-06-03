
# 🚀 Tutoriel Configuration Cloud : TTN ➔ TagoIO ➔ API Render

> ⚠️ **IMPORTANT : AVANT DE COMMENCER**
> Avant de suivre ce tutoriel, vous devez obligatoirement avoir configuré et lancé les composants matériels et locaux. 
> Suivez d'abord les guides détaillés dans chaque sous-dossier :
> 1. Configurer et câbler l'électronique ➔ Voir le `README.md` du dossier **`/arduino`**
> 2. Installer et lancer la base de données et l'API ➔ Voir le `README.md` du dossier **`/server`**

---

## 1. Configuration sur The Things Network (TTN)

La passerelle LoRaWAN du FabLab capte le signal de notre carte, mais il faut créer une application sur TTN pour récupérer et décoder la donnée.

### Étape 1 : Créer l'application et le périphérique
1. Connecte-toi sur la console [The Things Network](https://eu1.thethings.network/console/).
2. Va dans **Applications** ➔ **+ Add application** et donne-lui un nom (ex: `projet-presence-l1`).
3. Dans l'application, clique sur **Register device** :
   * **Activation mode** : Sélectionne **ABP** (comme configuré dans notre code Arduino).
   * Sisis les clés qui correspondent exactement à ton code Arduino : `DevAddr`, `NwkSKey`, et `AppSKey`.

### Étape 2 : Ajouter le Payload Formatter (Décodeur)
Par défaut, l'Arduino envoie des octets bruts (de l'hexadécimal). Il faut dire à TTN comment transformer ces octets en texte lisible.
1. Dans le menu de gauche de ton application TTN, clique sur **Payload formatters** ➔ **Uplink**.
2. Sélectionne **Javascript** et remplace le code par celui-ci pour reconstruire l'UID de la carte :

```javascript
function decodeUplink(input) {
  var bytes = input.bytes;
  var uid = "";
  for (var i = 0; i < bytes.length; i++) {
    var hex = bytes[i].toString(16).toUpperCase();
    if (hex.length < 2) hex = "0" + hex;
    uid += hex;
  }
  return {
    data: {
      uid_carte: uid
    },
    warnings: [],
    errors: []
  };
}

```

3. Sauvegarde. Désormais, quand tu penses une carte, TTN affichera proprement : `"uid_carte": "7BF81607"`.

---

## 2. Configuration sur TagoIO

TagoIO va recevoir la donnée de TTN, appeler notre serveur Render pour identifier l'étudiant, et l'afficher sur un Dashboard.

### Étape 1 : Connecter TTN à TagoIO (Liaison Device)

1. Crée un compte sur [TagoIO](https://tago.io/).
2. Va dans **Devices** ➔ **+ Add Device**.
3. Cherche le connecteur **The Things Network / LoRaWAN** et sélectionne **Custom TTN**.
4. Donne un nom à ton appareil (ex: `Boitier_RFID`) et saisis le `DevEUI` de ton projet.
5. Génère un **Authorization Token** dans TagoIO et colle-le dans la section *Webhooks* de TTN pour lier les deux plateformes.

---

## 3. Le Code du Script "Analysis" (Node.js)

L'**Analysis** est un script de code exécuté directement sur le Cloud de TagoIO. Son rôle est d'intercepter l'UID, de l'envoyer à notre serveur Render, et d'enregistrer le résultat sur TagoIO.

1. Sur TagoIO, va dans **Analysis** ➔ **+ New Analysis** (Choisis l'environnement *Node.js*).
2. Colle le code suivant dans l'éditeur de TagoIO :

```javascript
import pkg from "@tago-io/sdk";

const { Analysis, Device, Utils } = pkg;

async function myAnalysis(context, scope) {

  try {

    const card = scope.find(x => x.variable === "uid");

    if (!card) {
      context.log("Aucun UID trouvé.");
      return;
    }

    console.log("UID reçu :", card.value);

    const response = await fetch(
      "lurl de ton serveur heberge",
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({
          cartId: card.value
        })
      }
    );

    if (!response.ok) {
      throw new Error(`Erreur HTTP ${response.status}`);
    }

    const data = await response.json();

    console.log("Réponse API :", data);

    const environment = Utils.envToJson(context.environment);

    // Vérifie le token
    if (!environment.device_token) {
      context.log("device_token manquant.");
      return;
    }

    // Connexion Device
    const device = new Device({
      token: environment.device_token
    });

    const variablesToSend = [
      {
        variable: "username",
        value: data.noms || "Inconnu",
      },
      {
        variable: "firstname",
        value: data.prenoms || "Inconnu",
      },
      {
  variable: "autorisation",
  value: data.autorisation || "Inconnu",
    
    
  })
}
      
    ];

    console.log("Envoi :", variablesToSend);

    await device.sendData(variablesToSend);

    context.log(
      `Données enregistrées pour ${data.prenom || "Inconnu"}`
    );

  } catch (error) {

    context.log(`Erreur : ${error.message}`);

  }
}

export default new Analysis(myAnalysis);

```

---

## 4. Configuration de l'Action (Le Déclencheur)

Pour que le script **Analysis** s'exécute automatiquement à chaque fois qu'un étudiant badge, il faut configurer une **Action** sur TagoIO.

1. Va dans **Actions** ➔ **+ New Action**.
2. Configure les paramètres suivants :
* **Action Name** : `Declencheur Scan RFID`.
* **Trigger Type** : Sélectionnez **Variable**.
* **Device** : Choisis ton `Boitier_RFID`.
* **Condition** : Si la variable `uid` **Any** (reçoit n'importe quelle valeur).


3. Dans l'onglet **Type of Action** (en haut) :
* Sélectionne **Run Analysis**.
* Choisis le script Analysis que tu as créé à l'étape précédente.


4. Clique sur **Save**.

### 🎯 Résultat final attendu

Quand une carte passe sur le lecteur RFID :

1. L'Arduino envoie l'UID brut via **LoRaWAN**.
2. **TTN** le décode en texte clair.
3. **TagoIO** reçoit la variable `uid`.
4. L'**Action** détecte le changement et lance l'**Analysis**.
5. L'**Analysis** interroge **Render + MongoDB**, récupère l'identité (Ex: *Fadel Koudokodji*) et l'affiche instantanément sur ton widget de texte sur le Dashboard TagoIO !

```

```
