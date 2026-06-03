const Utilisateur = require("../model/User");




async function creerUtilisateur(req, res) {
    const { cartId, noms, prenoms, email,role } = req.body;
    if (!cartId || !name || !surname || !email|| !role) {
        return res.status(400).json({ message: "vous devez remplir tous les champs" });

    }
    try {
        const existingUser = await Utilisateur.findOne({ cartId });
        if (existingUser) {
            return res.status(400).json({ message: "Un utilisateur avec ce cartId existe déjà" });
        }
        const utilisateur = new Utilisateur({ cartId, noms, prenoms, email,role });
        await utilisateur.save();
        res.status(201).json({ message: "Utilisateur créé avec succès", utilisateur });
    } catch (error) {
        console.error(error);
        res.status(500).json({ message: "Erreur lors de la création de l'utilisateur" });
    }
}

async function userInfo(req, res) {
    const { cartId } = req.body;

    if (!cartId) {
        return res.status(400).json({
            message: "cartId est requis"
        });
    }

    try {
        const utilisateur = await Utilisateur.findOne({ cartId });

        if (!utilisateur) {
            return res.status(404).json({
                message: "Utilisateur non trouvé"
            });
        }

        
        let autorisation;

        if (utilisateur.role === "responsable") {
            autorisation = "Autorisée";
        } else {
            autorisation = "Refusée";
        }

        res.status(200).json({
            message: "Informations de l'utilisateur",
            noms: utilisateur.noms,
            prenoms: utilisateur.prenoms,
            autorisation: autorisation,
            success: true,
            error: false
        });

    } catch (error) {

        res.status(500).json({
            message: "Erreur lors de la récupération des informations de l'utilisateur",
            success: false,
            error: true
        });
    }
}
module.exports = {
    creerUtilisateur,
    userInfo
}
