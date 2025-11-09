const { app } = require("@azure/functions");
const { CosmosClient } = require("@azure/cosmos");

//Configs
const endpoint = process.env.COSMOS_DB_ENDPOINT;
const key = process.env.COSMOS_DB_KEY;
const databaseId = "db1-irrigador";
const containerId = "Container1";

app.http("getData", {
  methods: ["GET"],
  authLevel: "anonymous",
  route: "getLastData/",
  handler: async (request, context) => {
    context.log(`Http function processed request for url "${request.url}"`);
    const id = request.params.id;
    try {
      // Cria cliente Cosmos DB
      const client = new CosmosClient({ endpoint, key });
      const container = client.database(databaseId).container(containerId);

      const query = {
        query: "SELECT TOP 1 * FROM c ORDER BY c._ts DESC",
        // parameters: [{ name: "@id", value: id }],
      };

      const { resources: results } = await container.items
        .query(query)
        .fetchAll();

      if (results.length === 0) {
        return {
          status: 404,
          body: `Nenhum documento encontrado com id ${id}`,
        };
      } else {
        return {
          status: 200,
          body: JSON.stringify(results[0]),
        };
      }
    } catch (error) {
      context.error("Erro ao acessar o Cosmos DB:", error.message);
      return {
        status: 500,
        body: "Erro interno ao consultar o banco de dados.",
      };
    }
  },
});
