const mongoose = require("mongoose");
const validator = require("validator");

const userSchema = new mongoose.Schema({
    cartId: {
        type: String,
        required: true,
        unique: true
    },
    noms: {
        type: String,
        required: true
    },
    prenoms: {
        type: String,
        required: true
    },
    email: {
        type: String,
        required: true,
        unique: true,
        validate: {
            validator: function(value){
                return validator.isEmail(value);
            },
            message: "Invalid email format"
        }
    },
    role:{
        type:String,
        required:true
    },
    createdAt: {
        type: Date,
        default: Date.now
    }
});




module.exports = mongoose.model("Utilisateur", userSchema);
