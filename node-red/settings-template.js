module.exports = {
    uiPort: process.env.PORT || 1880,

    adminAuth: {
        type: "credentials",
        users: [{
            username: "__USERNAME__",
            password: "__PASSWORD_HASH__",
            permissions: "*"
        }]
    },

    httpNodeAuth: {
        user: "__USERNAME__",
        pass: "__PASSWORD_HASH__"
    },

    functionGlobalContext: {},

    logging: {
        console: {
            level: "info",
            audit: false
        }
    }
};
