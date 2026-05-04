FROM nodered/node-red:latest

# Install palette nodes
RUN npm install --no-optional \
    node-red-node-mysql \
    @flowfuse/node-red-dashboard

USER root

# Create entrypoint inline
RUN cat <<'ENTRY' > /usr/src/node-red/entrypoint.sh
#!/bin/sh
set -e
if [ ! -f /data/settings.js ]; then
  echo "==> Generating settings.js with adminAuth..."
  HASH=$(node -e "const b=require('bcryptjs');console.log(b.hashSync(process.env.NODE_RED_PASSWORD||'admin',8))")
  sed "s|__USERNAME__|${NODE_RED_USER:-admin}|g;s|__PASSWORD_HASH__|${HASH}|g" \
    /usr/src/node-red/settings-template.js > /data/settings.js
  echo "==> Done. User: ${NODE_RED_USER:-admin}"
fi
exec npm start -- --userDir /data "$@"
ENTRY

# Create settings template inline
RUN cat <<'SETTINGS' > /usr/src/node-red/settings-template.js
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
    logging: { console: { level: "info", audit: false } }
};
SETTINGS

RUN chmod +x /usr/src/node-red/entrypoint.sh

USER node-red

ENTRYPOINT ["/usr/src/node-red/entrypoint.sh"]
