#include "cloud-provider.h"
#include "clova/clova-provider.h" // Include your specific cloud provider headers

std::shared_ptr<CloudProvider> createCloudProvider(const std::string &providerType,
						   CloudProvider::TranscriptionCallback callback,
						   cloudvocal_data *gf)
{
	if (providerType == "clova") {
		return std::make_shared<ClovaProvider>(callback, gf);
	}
	// Add more providers as needed
	// else if (providerType == "AnotherCloudProvider") {
	//     return std::make_unique<AnotherCloudProvider>(callback, gf);
	// }

	return nullptr; // Return nullptr if no matching provider is found
}
