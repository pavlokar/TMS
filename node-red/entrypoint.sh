#!/bin/sh
set -e

# Generate settings.js with bcrypt hash on first start
if [ ! -f /data/settings.js ]; then
  echo "==> Generating settings.js with adminAuth..."
  HASH=$(node -e "const b=require('bcryptjs');console.log(b.hashSync(process.env.NODE_RED_PASSWORD||'admin',8))")
  sed "s|__USERNAME__|${NODE_RED_USER:-admin}|g;s|__PASSWORD_HASH__|${HASH}|g" \
    /usr/src/node-red/settings-template.js > /data/settings.js
  echo "==> Done. User: ${NODE_RED_USER:-admin}"
fi

exec npm start -- --userDir /data "$@"