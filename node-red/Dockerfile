FROM nodered/node-red:latest

USER root

RUN npm install --omit=optional \
    node-red-node-mysql \
    @flowfuse/node-red-dashboard

COPY --chmod=755 entrypoint.sh /entrypoint.sh
COPY settings-template.js /usr/src/node-red/settings-template.js

USER node-red

ENTRYPOINT ["/entrypoint.sh"]