#include "cloudvocal.h"

struct obs_source_info cloudvocal_info = {
	.id = "cloudvocal_audio_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = cloudvocal_name,
	.create = cloudvocal_create,
	.destroy = cloudvocal_destroy,
	.get_defaults = cloudvocal_defaults,
	.get_properties = cloudvocal_properties,
	.update = cloudvocal_update,
	.activate = cloudvocal_activate,
	.deactivate = cloudvocal_deactivate,
	.filter_audio = cloudvocal_filter_audio,
	.filter_remove = cloudvocal_remove,
	.show = cloudvocal_show,
	.hide = cloudvocal_hide,
};
