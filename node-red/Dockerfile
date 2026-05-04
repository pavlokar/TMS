FROM nodered/node-red:latest

RUN npm install --no-optional \
    node-red-node-mysql \
    @flowfuse/node-red-dashboard

COPY --chmod=755 entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]