local input_dir = '../test/asset/image2'
local output_dir = '../output/asset/image'

local converted_count = 0
local failed_count = 0
local skipped_count = 0

local function ensureDir(dir)
    if not love.filesystem.getInfo(dir) then
        love.filesystem.createDirectory(dir)
    end
end

local function traverse(dir, rel_path)
    rel_path = rel_path or ''
    local current_dir = dir .. '/' .. rel_path
    
    local files = love.filesystem.getDirectoryItems(current_dir)
    for _, entry in ipairs(files) do
        if entry ~= '.' and entry ~= '..' then
            local entry_path = rel_path .. '/' .. entry
            local full_path = dir .. '/' .. entry_path
            local info = love.filesystem.getInfo(full_path)
            
            if info and info.type == 'directory' then
                love.filesystem.createDirectory(output_dir .. '/' .. entry_path)
                traverse(dir, entry_path)
            elseif string.match(entry, '%.pkm$') then
                local output_path = output_dir .. '/' .. string.sub(entry_path, 1, -4) .. '.png'
                
                if love.filesystem.getInfo(output_path) then
                    skipped_count = skipped_count + 1
                    print('Skipping (exists): ' .. entry_path)
                else
                    print('Processing: ' .. entry_path)
                    
                    local fileData, err = love.filesystem.newFileData(full_path)
                    if fileData then
                        local success, compressedData = pcall(love.image.newCompressedData, fileData)
                        if success and compressedData then
                            local success2, image = pcall(love.graphics.newImage, compressedData)
                            if success2 and image then
                                local imageData = image:getData()
                                local pngData = imageData:encode('png')
                                love.filesystem.write(output_path, pngData)
                                converted_count = converted_count + 1
                                print('  -> Converted successfully')
                            else
                                failed_count = failed_count + 1
                                print('  -> Failed to create image')
                            end
                        else
                            failed_count = failed_count + 1
                            print('  -> Failed to decode compressed data (ETC2 not supported?)')
                        end
                    else
                        failed_count = failed_count + 1
                        print('  -> Failed to read file')
                    end
                end
            end
        end
    end
end

function love.load()
    print('Starting batch conversion from KTX/ETC2 to PNG...')
    print('Input: ' .. input_dir)
    print('Output: ' .. output_dir)
    print('----------------------------------------')
    
    ensureDir(output_dir)
    traverse(input_dir)
    
    print('----------------------------------------')
    print('Conversion complete!')
    print('Converted: ' .. converted_count)
    print('Failed: ' .. failed_count)
    print('Skipped: ' .. skipped_count)
    
    love.event.quit()
end

function love.draw()
    love.graphics.print('Conversion complete', 10, 10)
end

function love.keypressed()
    love.event.quit()
end
