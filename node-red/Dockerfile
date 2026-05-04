FROM nodered/node-red:latest

# Install palette nodes at build time
RUN npm install --no-optional \
    node-red-node-mysql \
    @flowfuse/node-red-dashboard

# Copy entrypoint and settings template
COPY entrypoint.sh /usr/src/node-red/entrypoint.sh
COPY settings.js /usr/src/node-red/settings-template.js

USER root
RUN chmod +x /usr/src/node-red/entrypoint.sh
USER node-red

ENTRYPOINT ["/usr/src/node-red/entrypoint.sh"]